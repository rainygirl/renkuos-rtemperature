# rtemperature

[English](README.md) | [한국어](README.ko.md)

Haiku OS용 Deskbar 트레이 온도계입니다. 시계 옆에 머신 온도를 표시하며, 그 외에는
아무 곳에도 나타나지 않습니다. 창도 없고 Deskbar 팀 목록에도 항목이 없습니다.
트레이 항목은 BeOS 스타일의 작은 온도계로, 온도에 따라 수은주가 올라가고 그
옆에 섭씨 값이 표시됩니다.

![Deskbar 트레이의 rtemperature 항목](docs/deskbar-tray.png)

숫자는 항상 트레이의 기본 메뉴 글자색으로 그려지고, 온도에 따라 색이 변하는 것은
수은주뿐입니다.

## 기본(gcc2) 컴파일러로 빌드하기

x86 하이브리드 Haiku에서 Deskbar는 x86_gcc2 바이너리이며, 같은 아키텍처의
애드온만 로드할 수 있습니다. 기본 `g++`(gcc2)로 빌드하세요. 최신 gcc로 빌드하면
Deskbar가 리플리컨트를 로드할 때 크래시가 납니다. 소스는 gcc2 호환으로
유지하고 있습니다.

## 빌드

```
make
```

Be API와 `rc`/`xres`/`mimeset`을 사용하므로 Haiku에서 빌드해야 합니다.

## 사용법

```
./rtemperature            # 트레이에 추가
./rtemperature --remove   # 트레이에서 제거
./rtemperature --print    # 현재 온도를 출력하고 종료
```

## 온도 값의 출처

커널은 ACPI 열 영역(thermal zone)을 `/dev/power/acpi_thermal/N`에 공개합니다.
이 디바이스는 짧은 텍스트 리포트를 반환하므로, Haiku 버전마다 바뀌어 온 구조체
레이아웃에 의존하지 않고 `Current Temperature` 줄을 파싱합니다. 읽기는 한 번만,
논블로킹으로 수행합니다. 이 디바이스는 두 번째 읽기에서 블로킹되며 그러면
Deskbar의 윈도우 스레드가 멈추기 때문입니다.

트레이에 `--`가 표시된다면 이 머신에는 읽을 수 있는 ACPI 열 영역이 없는
것입니다.

## 라이선스

MIT

## AI 고지

이 프로그램은 Claude로 작성되었습니다.
