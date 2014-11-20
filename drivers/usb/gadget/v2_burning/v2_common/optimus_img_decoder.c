/*
 * \file        optimus_img_decoder.c 
 * \brief       
 *
 * \version     1.0.0
 * \date        2013-7-8
 * \author      Sam.Wu <yihui.wu@amlogic.com>
 *
 * Copyright (c) 2013 Amlogic. All Rights Reserved.
 *
 */
#include "../v2_burning_i.h"

#define IMAGE_MAGIC	                0x27b51956	 /* Image Magic Number		*/
#define VERSION                     0x0001

#define COMPILE_TYPE_CHK(expr, t)       typedef char t[(expr) ? 1 : -1]
//FIMXE:
COMPILE_TYPE_CHK(128 == sizeof(ItemInfo), a);
COMPILE_TYPE_CHK(64  == sizeof(AmlFirmwareImg_t), b);

typedef struct _ImgInfo_s
{
    AmlFirmwareImg_t    imgHead;
    ItemInfo            itemInfo[64];

}ImgInfo_t;

static int _hFile = -1;

//open a Amlogic firmware image
//return value is a handle
HIMAGE image_open(const char* imgPath)
{
    const int HeadSz = sizeof(ImgInfo_t);
    ImgInfo_t* hImg = NULL;
    int ret = 0;

    hImg = (ImgInfo_t*)malloc(HeadSz);
    if(!hImg){
        DWN_ERR("Fail to malloc %d\n", HeadSz);
        return NULL;
    }

    int pFile = do_fat_fopen(imgPath);
    if(pFile < 0){
        DWN_ERR("Fail to open file %s\n", imgPath);
        goto _err;
    }
    _hFile = pFile;

    ret = do_fat_fread(pFile, (u8*)hImg, HeadSz);
    if(ret != HeadSz){
        DWN_ERR("want to read %d, but %d\n", HeadSz, ret);
        goto _err;
    }

    if(IMAGE_MAGIC != hImg->imgHead.magic){
        DWN_ERR("error magic 0x%x\n", hImg->imgHead.magic);
        goto _err;
    }
    if(VERSION != hImg->imgHead.version){
        DWN_ERR("error verison 0x%x\n", hImg->imgHead.version);
        goto _err; 
    }

    return hImg;
_err:
    free(hImg); 
    return NULL;
}


//close a Amlogic firmware image
int image_close(HIMAGE hImg)
{
    DWN_MSG("to close image\n");

    if(_hFile >= 0)do_fat_fclose(_hFile), _hFile = -1;

    if(hImg){
        free(hImg); 
        hImg = NULL;
    }
    return 0;
}

//open a item in the image
//@hImage: image handle;
//@mainType, @subType: main type and subtype to index the item, such as ["IMAGE", "SYSTEM"]
HIMAGEITEM image_item_open(HIMAGE hImg, const char* mainType, const char* subType)
{
    ImgInfo_t* imgInfo = (ImgInfo_t*)hImg;
    const int itemNr = imgInfo->imgHead.itemNum;
    ItemInfo* pItem = imgInfo->itemInfo;
    int i = 0;

    for (; i < itemNr ;i++, pItem++)
    {
        if (!strcmp(mainType, pItem->itemMainType) && !strcmp(subType, pItem->itemSubType))
        {
            break;
        }
    }
    if(i >= itemNr){
        DWN_ERR("Can't find item [%s, %s]\n", mainType, subType);
        return NULL;
    }

    if(i != pItem->itemId){
        DWN_ERR("itemid %d err, should %d\n", pItem->itemId, i);
        return NULL;
    }

    DWN_DBG("Item offset 0x%llx\n", pItem->offsetInImage);
    i = do_fat_fseek(_hFile, pItem->offsetInImage, 0);
    if(i){
        DWN_ERR("fail to seek, offset is 0x%x\n", (u32)pItem->offsetInImage);
        return NULL;
    }
    
    return pItem;
}

//Need this if item offset in the image file is not aligned to bytesPerCluster of FAT
unsigned image_item_get_first_cluster_size(HIMAGEITEM hItem)
{
    ItemInfo* pItem = (ItemInfo*)hItem;
    unsigned itemSizeNotAligned = 0;
    const unsigned fat_bytesPerCluste = do_fat_get_bytesperclust(_hFile);

    itemSizeNotAligned = pItem->offsetInImage % fat_bytesPerCluste;
    itemSizeNotAligned = fat_bytesPerCluste - itemSizeNotAligned;

    DWN_DBG("itemSizeNotAligned 0x%x\n", itemSizeNotAligned);
    return itemSizeNotAligned;
}

//close a item
int image_item_close(HIMAGEITEM hItem)
{
    return 0;
}

__u64 image_item_get_size(HIMAGEITEM hItem)
{
    ItemInfo* pItem = (ItemInfo*)hItem;

    return pItem->itemSz;
}


//get image item type, current used type is normal or sparse
int image_item_get_type(HIMAGEITEM hItem)
{
    ItemInfo* pItem = (ItemInfo*)hItem;

    return pItem->fileType;
}

//read item data, like standard fread
int image_item_read(HIMAGE hImg, HIMAGEITEM hItem, void* pBuf, const __u32 wantSz)
{
    unsigned readSz = 0;

    readSz = do_fat_fread(_hFile, pBuf, wantSz);
    if(readSz != wantSz){
        DWN_ERR("want to read 0x%x, but 0x%x\n", wantSz, readSz);
        return __LINE__;
    }
    
    return 0;
}

int get_total_itemnr(HIMAGE hImg)
{
    ImgInfo_t* imgInfo = (ImgInfo_t*)hImg;

    return imgInfo->imgHead.itemNum;
}

HIMAGEITEM get_item(HIMAGE hImg, int itemId)
{
    int ret = 0;
    ImgInfo_t* imgInfo = (ImgInfo_t*)hImg;
    ItemInfo* pItem    = imgInfo->itemInfo + itemId;

    if(itemId != pItem->itemId){
        DWN_ERR("itemid %d err, should %d\n", pItem->itemId, itemId);
        return NULL;
    }
    DWN_MSG("get item [%s, %s] at %d\n", pItem->itemMainType, pItem->itemSubType, itemId);

    ret = do_fat_fseek(_hFile, pItem->offsetInImage, 0);
    if(ret){
        DWN_ERR("fail to seek, offset is 0x%x, ret=%d\n", (u32)pItem->offsetInImage, ret);
        return NULL;
    }
     
    return pItem;
}

int get_item_name(HIMAGE hImg, int itemId, const char** main_type, const char** sub_type)
{
    ImgInfo_t* imgInfo = (ImgInfo_t*)hImg;
    ItemInfo* pItem    = imgInfo->itemInfo + itemId;

    if(itemId != pItem->itemId){
        DWN_ERR("itemid %d err, should %d\n", pItem->itemId, itemId);
        return __LINE__;
    }
    DWN_DBG("get item [%s, %s] at %d\n", pItem->itemMainType, pItem->itemSubType, itemId);

    *main_type = pItem->itemMainType;
    *sub_type  = pItem->itemSubType;
     
    return OPT_DOWN_OK;
}

__u64 image_get_item_size_by_index(HIMAGE hImg, const int itemId)
{
    ImgInfo_t* imgInfo = (ImgInfo_t*)hImg;
    ItemInfo* pItem    = imgInfo->itemInfo + itemId;

    if(itemId != pItem->itemId){
        DWN_ERR("itemid %d err, should %d\n", pItem->itemId, itemId);
        return __LINE__;
    }
    DWN_DBG("get item [%s, %s] at %d\n", pItem->itemMainType, pItem->itemSubType, itemId);
     
    return pItem->itemSz;
}

u64 get_data_parts_size(HIMAGE hImg)
{
    int i = 0;
    int ret = 0;
    u64 dataPartsSz = 0;
    const int totalItemNum = get_total_itemnr(hImg);

    for (i = 0; i < totalItemNum; i++) 
    {
        const char* main_type = NULL;
        const char* sub_type  = NULL;

        ret = get_item_name(hImg, i, &main_type, &sub_type);
        if(ret){
            DWN_ERR("Exception:fail to get item name!\n");
            return __LINE__;
        }

        if(strcmp("PARTITION", main_type)) continue;
        if(!strcmp("bootloader", sub_type))continue;

        dataPartsSz += image_get_item_size_by_index(hImg, i);
    }

    return dataPartsSz;
}

#define MYDBG 0
#if MYDBG
int test_item(HIMAGE hImg, const char* main_type, const char* sub_type, char* pBuf, const int sz)
{
    HIMAGEITEM hItem = NULL;
    int ret = 0;

    hItem = image_item_open(hImg, main_type, sub_type);
    if(!hItem){
        DWN_ERR("fail to open %s, %s\n", main_type, sub_type);
        return __LINE__;
    }

    ret = image_item_read(hImg, hItem, pBuf, sz);
    if(ret){
        DWN_ERR("fail to read\n");
        goto _err;
    }
    if(64 >= sz)DWN_MSG("%s\n", pBuf);

_err:
    image_item_close(hItem);
    return ret;
}

int test_pack(const char* imgPath)
{
    const int ImagBufLen = OPTIMUS_DOWNLOAD_SLOT_SZ;
    char* pBuf = (char*)OPTIMUS_DOWNLOAD_TRANSFER_BUF_ADDR;
    int ret = 0;
    int i = 0;
    s64 fileSz = 0;
    HIMAGEITEM hItem = NULL;

    fileSz = do_fat_get_fileSz(imgPath);
    if(!fileSz){
        DWN_ERR("file %s not exist\n", imgPath);
        return __LINE__;
    }

    HIMAGE hImg = image_open(imgPath);
    if(!hImg){
        DWN_ERR("Fail to open image\n");
        return __LINE__;
    }

    const int itemNr = get_total_itemnr(hImg);
    for (i = 0; i < itemNr ; i++)
    {
        __u64 itemSz = 0;
        int fileType = 0;

        hItem = get_item(hImg, i);
        if(!hItem){
            DWN_ERR("Fail to open item at id %d\n", i);
            break;
        }

        itemSz = image_item_get_size(hItem);
        fileType = image_item_get_type(hItem);
        if(IMAGE_ITEM_TYPE_SPARSE == fileType)
        {
            unsigned wantSz = ImagBufLen > itemSz ? (unsigned)itemSz : ImagBufLen;

            DWN_MSG("sparse packet\n");
            ret = image_item_read(hImg, hItem, pBuf, wantSz);
            if(ret){
                DWN_ERR("fail to read item data\n");
                break;
            }

            ret = optimus_simg_probe((const u8*)pBuf, wantSz);
            if(!ret){
                DWN_ERR("item data error, should sparse, but no\n");
                break;
            }
        }
    }

#if 0
    test_item(hImg, "PARTITION", "logo", pBuf, ImagBufLen);
    test_item(hImg, "VERIFY", "logo", pBuf, 50);
#endif

    image_close(hImg);
    return 0;
}


int do_unpack(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int rcode = 0;
    const char* imgPath = argv[1];

#if 0
    if(2 > argc)imgPath = "dt.img";
#else
    if(2 > argc){
        cmd_usage(cmdtp);
        return -1;
    }
#endif//#if MYDBG
    DWN_MSG("argc %d, %s, %s\n", argc, argv[0], argv[1]);

    //mmc info to ensure sdcard inserted and inited
    rcode = run_command("mmcinfo", 0);
    if(rcode){
        DWN_ERR("Fail in init mmc, Does sdcard not plugged in?\n");
        return __LINE__;
    }

    rcode = test_pack(imgPath);

    DWN_MSG("rcode %d\n", rcode);
    return rcode;
}

U_BOOT_CMD(
   unpack,      //command name
   5,               //maxargs
   1,               //repeatable
   do_unpack,   //command function
   "unpack the image in sdmmc ",           //description
   "Usage: unpack imagPath\n"   //usage
);
#endif//#if MYDBG

