#ifndef HEART_BEAT_H
#define HEART_BEAT_H

/*used by tracker and storage*/
/*count_stamp_pool of storage && count_stamp for storage*/
#define GROUP_MAXSIZE 			10
#define STORAGE_MAXSIZE 		5

#define STORAGE_OFFLINE 		0
#define STORAGE_ACTIVE  		1
#define STORAGE_ONLINE  		2
//#define STORAGE_ACTIVE_OR_ONLINE 	(STORAGE_ACTIVE|STORAGE_ONLINE)
//#define STORAGE_SYNCED		4
#define STORAGE_RECOVERY		4
#define STORAGE_RECOVERYED		5
#define STORAGE_DEAD			6

#define GROUP_OFFLINE			0
#define GROUP_ONLINE			1

typedef struct STORAGE_ADDRESS_INFO_T
{
	in_addr_t ip;
	unsigned short port;	
}storage_address_info_t;

typedef struct COUNT_STAMP_T
{
	char storage[STORAGE_MAXSIZE][9];
}count_stamp_t;

typedef struct NEW_ONLINE_STORAGE_T
{
	storage_address_info_t storage_address_info;
	char storage_id;
	char if_need_recovery;
}new_storage_t;

typedef struct RETURN_HEART_BEAT_BODY_T
{
	new_storage_t *new_storage;
}return_heart_beat_body_t;

typedef struct RETURN_HEART_BEAT_HEAD_T
{
	char storage_id;
	char unknow_new_num;	
	char unknow_dea_num;
	char status;
}return_heart_beat_head_t;

typedef struct RETURN_HEART_BEAT_T
{
	return_heart_beat_head_t head;
	return_heart_beat_body_t body;
}return_heart_beat_t;

typedef struct HEART_BEAT_T
{
	long long storage_size;
	long long upload_size;
	count_stamp_t count_stamp;
	char know_sf[STORAGE_MAXSIZE];
	unsigned short port;
	char status;
	char group_id;
	char storage_id;
}heart_beat_t;
/*end*/

#endif
