#pragma once
#include "framework.h"
#include "resource.h"

#define REG_COUNT 57					// MFC_HR[0] ~ MFC_HR[56]
#define TIMER_ID 1
#define READ_INTERVAL 500

/* 장비 연결 상태 */
#define ARDUINO_STATE_IDX 10			// 아두이노 연결 신호 = MFC_HR[10]
#define ARDUINO_STATE_TARGET 12			// 연결 중일 때 값 = WORD#16#000C

#define ROBOT_STATE_IDX 11				// 로봇 상태 (두산 Robot State 값 그대로) = MFC_HR[11]
#define ROBOT_STATE_TARGET 15			// 연결 중 값 1 = WORD#16#000F (서보 on / 태스크 정지)
#define ROBOT_TASK_TARGET 16			// 연결 중 값 2 = WORD#16#0010 (태스크 실행 중)

/* 로봇 작업 상태 */
#define ROBOT_KIND_IDX 38				// 작업 종류 = MFC_HR[38] (0 : 대기 / 1 : 대여 / 2 : 반납 / 3 : 고장 회수)
#define ROBOT_STEP_IDX 39				// 작업 단계 = MFC_HR[39] (1 접수 / 2 이동 / 3 파지 / 4 이동 / 5 배출·삽입 / 6 완료)

/* 로봇 대여 및 반납 완료 */
#define RENT_SLOT_IDX 12				// 마지막으로 대여한 슬롯 번호 = MFC_HR[12]
#define RENT_SEQ_IDX 13					// 대여 완료될 때마다 +1 = MFC_HR[13]
#define RETURN_SLOT_IDX 14				// 마지막으로 반납한 슬롯 번호 = MFC_HR[14]
#define RETURN_SEQ_IDX 15				// 반납 완료될 때마다 +1 = MFC_HR[15]

/* 슬롯 테이블 */
#define SLOT_TABLE_IDX 17				// 슬롯 테이블 시작 위치 = MFC_HR[17] (Slot_No[0] ~ [6], 17 ~ 37)
#define SLOT_REG_COUNT 3				// 슬롯당 레지스터 개수
#define SLOT_STATUS_OFS 0				// 0 : 비어 있음 / 1 : 충전 중 / 2 : 불량
#define SLOT_SOC_OFS 1					// 배터리 잔량 %
#define SLOT_ROBOTIO_OFS 2				// 로봇 IO
#define SLOT_REG(s, ofs) (SLOT_TABLE_IDX + (s) * SLOT_REG_COUNT + (ofs))	// Slot_No[s]의 위치
#define SOC_FAULT 110

#define SLOT_COUNT 7					// Slot_No[0] ~ [6] (구조체 배열 크기)
#define SLOT_FIRST 1					// Slot_No[1]번부터 사용
#define SLOT_LAST 6						// Slot_No[6]이 마지막

/* 슬롯 상태 값 */
#define SLOT_EMPTY 0
#define SLOT_CHARGING 1
#define SLOT_FAULTY 2

/* 배터리 */
#define BATTERY_COUNT 6 				// 배터리 ID 1 ~ 6
#define BATTERY_ID_IDX 40				// MFC_HR[40]~[45] = 슬롯 1~6에 꽂힌 배터리 ID (0 = 없음)
#define RENT_BATID_IDX 46				// 마지막으로 대여된 배터리 ID = MFC_HR[46]
#define RENT_BITS_IDX 48				// 대여 중 비트 = MFC_HR[48] (bit0 = 배터리 1 … bit5 = 배터리 6)
#define RENT_SEC_IDX 51					// MFC_HR[51]~[56] = 배터리 ID 1~6 대여 경과 시간(초)

/* HMI 표시 기준 */
#define RENT_PCT_MIN 60					// 이 잔량(%) 이상이면 대여 가능

enum { LED_PLC, LED_VISION, LED_ROBOT, LED_ARDUINO, LED_COUNT };
static const UINT kLedIds[LED_COUNT] = { IDC_PLC_LED, IDC_Vision_LED, IDC_Robot_LED, IDC_Arduino_LED };
static const int kLEDRow[LED_COUNT] = { 0, 2, 3, 4 };