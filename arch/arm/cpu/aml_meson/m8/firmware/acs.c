#include <asm/arch/acs.h>

#ifdef CONFIG_ACS

//main acs struct
struct acs_setting __acs_set={
					.acs_magic		= "acs_",
					//chip id, m6:0x22 m8:0x24
					.chip_type		= 0x24,
					.version 		= 1,
					.acs_set_length	= sizeof(__acs_set),

					.ddr_magic		= "ddr_",
					.ddr_set_version= 1,
					.ddr_set_length	= sizeof(__ddr_setting),
					.ddr_set_addr	= &__ddr_setting,

					.pll_magic		= "pll_",
					.pll_set_version= 2,
					.pll_set_length	= sizeof(__plls),
					.pll_set_addr	= &__plls,

					.partition_table_magic		= "part",
					.partition_table_version	= 1,
					.partition_table_length		= (MAX_PART_NUM*sizeof(struct partitions)),
					.partition_table_addr		= & partition_table,

					.store_config_magic 		="stor",
					.store_config_version		= 1,
					.store_config_length		= sizeof(store_configs),
					.store_config_addr		=& store_configs,
};

#endif
