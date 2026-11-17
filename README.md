## 보조 배터리 공유 서비스 자동화 프로젝트 - 피카 츙전
---

### 프로젝트 개요
본 프로젝트는 기존 보조 배터리 대여 시스템의 이용 불편과 충전 상태 관리 문제를 개선하고자 한다.
배터리와 충전 단자의 결합 상태와 충전 상태를 자동으로 확인하고, 정상적으로 충전된 배터리만 대여/반납 할 수 있는 자동화 시스템을 구현하고자 하였다.

---

## Tech Stack

![C++](https://img.shields.io/badge/C++-00599C?style=flat&logo=cplusplus&logoColor=white)
![Visual Studio](https://img.shields.io/badge/Visual%20Studio-5C2D91?style=flat&logo=visualstudio&logoColor=white)
![Arduino](https://img.shields.io/badge/Arduino%20Mega2560-00878F?style=flat&logo=arduino&logoColor=white)
![Sysmac Studio](https://img.shields.io/badge/Sysmac%20Studio-005EB8?style=flat)
![DRL](https://img.shields.io/badge/Doosan%20DRL-0072CE?style=flat)

### Software
| 구분 | 사용 툴 / 언어 | 용도 |
|---|---|---|
| PC 프로그램 | Visual Studio / MFC (C++) | 전체 시스템 제어 및 모니터링 GUI |
| PLC | Omron Sysmac Studio | 컨베이어·센서·공정 시퀀스 제어 |
| 로봇 | Doosan Robotics DRL | 로봇 동작 및 작업 시퀀스 프로그래밍 |
| 펌웨어 | Arduino IDE | RFID 인식, 배터리 잔량 측정 및 LCD 표시 |

### Hardware
| 장비 | 모델 | 용도 |
|---|---|---|
| 협동 로봇 | Doosan (A0509) | 배터리 pick&place |
| PLC | Omron (NX102-1020) | 공정 제어 |
| HMI | Omron (NA5-9W001B) | 터치 패널 (UI) |
| Vision | Omron (FZ-SQ050F) | 비전 카메라 |
| MCU | Arduino Mega2560 | 서브 컨트롤러 |
| RFID 모듈 | RC522 | 태그 인식 |
| 배터리 측정 | 전압 센서 모듈 (INA219) | 배터리 전압·잔량 측정 |
| 디스플레이 | 16x2 I2C LCD | 결제 상태 및 정보 표시 |

---

프로젝트 기획 배경, 시스템 구성, 모델 학습 과정 및 구현 상세 내용은 아래 블로그에 정리되어 있습니다.
[프로젝트 상세 설명 보러 가기] (https://xiin.tistory.com/48)
