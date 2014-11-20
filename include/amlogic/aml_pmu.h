
#ifndef __AML_PMU_H__
#define __AML_PMU_H__

/*
 * function declaration
 */
int aml_pmu_write  (int32_t add, uint8_t val);                    // single write
int aml_pmu_write16(int32_t add, uint32_t val);                    // 16bit register write
int aml_pmu_writes (int32_t add, uint8_t *buff, int len);     // block write
int aml_pmu_read   (int32_t add, uint8_t *val);
int aml_pmu_read16 (int32_t add, uint16_t *val);
int aml_pmu_reads  (int32_t add, uint8_t *buff, int len);

int aml_pmu_is_ac_online(void);
void aml_pmu_power_off(void);
int aml_pmu_get_charging_percent(void);
int amp_pmu_set_charging_current(int current);

int aml_pmu_set_gpio(int pin, int val);
int aml_pmu_get_gpio(int pin, int *val);

#ifdef CONFIG_UBOOT_BATTERY_PARAMETER_TEST
int aml_battery_calibrate(void);
#endif

#endif  /* __AML_PMU_H__ */

