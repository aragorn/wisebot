/* $Id$ */
#ifndef MOD_DOCATTR_LGCALTEX_H
#define MOD_DOCATTR_LGCALTEX_H 1

#include <string.h>

#define STRUCTURE_CODE_LEN 20
#define PERSON_CODE_LEN 10
#define TFT_CODE_LEN 10
#define MAX_POLICY 256

typedef struct{
    int type;

    union {
		char structure[STRUCTURE_CODE_LEN];
		int duty;
		char person[PERSON_CODE_LEN];
		char tft[TFT_CODE_LEN];
    }policy;
} lgcaltex_policy_t;

typedef struct {
	int count;
	lgcaltex_policy_t* policy;
} lgcaltex_vrm_t;

/* must smaller than 64 byte */
typedef struct {
	uint8_t is_deleted:1;
	uint8_t SystemName:6;
	uint8_t AppFlag:1;
	
	uint8_t Part:4; 
	uint8_t FileExt:4; 
		
	uint8_t dummay:3; 
	uint8_t SC:1;
	uint8_t StrYN:1;
	uint8_t DutyYN:1;
	uint8_t PerYN:1;
	uint8_t TFTYN:1;
		
	uint32_t MILE:32; 
	 
	uint32_t Date1:32;
	 
	uint32_t Date2:32; 

	char Title[16];
	char Author[16];
	char rsv2[17];
} __attribute__((packed)) lgcaltex_attr_t;


/* must smaller than STRING_SIZE(256) byte */
typedef struct {
	uint8_t delete_check;

	uint8_t SystemName_check;
	uint8_t SystemName[60];

	uint8_t Part_check;
	uint8_t Part;

	uint8_t FileExt_check;
	uint8_t FileExt;
	
	uint8_t Duty_check;
	int	Duty_cnt;
	uint8_t Duty;
	
	uint8_t Structure_check;
	int	Structure_cnt;
	char 	**Structure;
	
	uint8_t Person_check;
	char Person[20];
	
	uint8_t TFT_check;
	int	TFT_cnt;
	char 	**TFT;
	
	uint8_t Date1_check;
	uint32_t Date1_start;
	uint32_t Date1_finish;

	uint8_t Date2_check;
	uint32_t Date2_start;
	uint32_t Date2_finish;
	
	uint8_t  Super_User;
	
} lgcaltex_cond_t;   /* 검색시 조건 입력 */


/* must smaller than STRING_SIZE(256) byte */
typedef struct {
	uint8_t delete_mark;
	uint8_t undelete_mark;

	uint8_t set_SystemName;
	uint8_t SystemName;

	uint8_t set_AppFlag;
	uint8_t AppFlag;
	
	uint8_t set_Part;
	uint8_t Part;

	uint8_t set_FileExt;
	uint8_t FileExt;
	
	//uint8_t set_Duty;
	//uint8_t Duty;
	
	uint8_t set_SC;
	uint8_t SC;
	
	uint8_t set_StrYN;
	uint8_t StrYN;
	
	uint8_t set_DutyYN;
	uint8_t DutyYN;
	
	uint8_t set_PerYN;
	uint8_t PerYN;
	
	uint8_t set_TFTYN;
	uint8_t TFTYN;
	
	uint8_t set_MILE;
	uint32_t MILE;
	
	uint8_t set_Date1;
	uint32_t Date1;

	uint8_t set_Date2;
	uint32_t Date2;

	uint8_t set_Title;
	char Title[16];
	
	uint8_t set_Author;
	char Author[16];

} lgcaltex_mask_t;
 

#endif
