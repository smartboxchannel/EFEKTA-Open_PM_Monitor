# EFEKTA-Open_PM_Monitor

Описание: http://efektalab.com/Open_PM_Monitor

Поблагодарить автора: https://yoomoney.ru/fundraise/P2XLTgJsB6k.231012

Thank the author: https://yoomoney.ru/fundraise/P2XLTgJsB6k.231012

Телеграм чат DIY Devices - https://t.me/diy_devices

Продажа DIY Устройств - https://t.me/diydevmart

![EFEKTA Open PM Monitor](https://raw.githubusercontent.com/smartboxchannel/EFEKTA-Open_PM_Monitor/refs/heads/main/IMAGES/1.png)

Проект датчика мониторинга твердых частиц (PM1.0, PM2.5, PM10, PM2.5 Index) с аналоговым стрелочным дисплеем, Zigbee 3.0. Датчик является роутером сети.

Project of particle monitoring sensor (PM1.0, PM2.5, PM10, PM2.5 Index) with analog pointer display, Zigbee 3.0. The sensor is a network router.

![EFEKTA Open PM Monitor](https://raw.githubusercontent.com/smartboxchannel/EFEKTA-Open_PM_Monitor/refs/heads/main/IMAGES/123.png)

Сенсор твердых частиц, помимо данных PM2.5, также измеряет PM1, PM10 и рассчитывает PM2.5 Index. Проект сделан на основе сенсора ASAIR APM10.

The particle sensor, in addition to PM2.5 data, also measures PM1, PM10 and calculates PM2.5 Index. The project is based on the ASAIR APM10 sensor.

![EFEKTA Open PM Monitor](https://raw.githubusercontent.com/smartboxchannel/EFEKTA-Open_PM_Monitor/refs/heads/main/IMAGES/image%20(2).png)

Помимо стрелочной индикации, есть светодиодная индикация, которая отображает визуально уровень PM2.5 Index. Яркость светодиодов подсветки можно настраивать удалённо. Подсветку также можно отключить удалённо.

In addition to the arrow indication, there is an LED indication that visually displays the PM2.5 Index level. The brightness of the backlight LEDs can be adjusted remotely. The backlight can also be turned off remotely.

Датчик может обновляться новыми версиями прошивок по воздуху (OTA).

The sensor can be updated with new firmware versions over the air (OTA).

![EFEKTA Open PM Monitor](https://raw.githubusercontent.com/smartboxchannel/EFEKTA-Open_PM_Monitor/refs/heads/main/IMAGES/image%20(3).png)

## Основные данные:


- Identify - идентификация устройства

- State - включение/отключение подсветки

- Brightness - яркость подсветки

- Pm1 - уровень твердых частиц размером 1 мкм и менее

- Pm2.5 - уровень твердых частиц размером 2.5 мкм и менее

- Pm10 - уровень твердых частиц размером 10 мкм и менее

- Aqi25 - индекс качества воздуха на основе PM2.5 (EPA)

## Basic data:

- Identify - device identification

- State - backlight on/off

- Brightness - backlight brightness

- Pm1 - particulate matter level of 1 µm or less

- Pm2.5 - particulate matter level of 2.5 µm or less

- Pm10 - particulate matter level of 10 µm or less

- Aqi25 - air quality index based on PM2.5 (EPA)

  ![EFEKTA Open PM Monitor](https://raw.githubusercontent.com/smartboxchannel/EFEKTA-Open_PM_Monitor/refs/heads/main/IMAGES/image%20(4).png)

## Конфигурационные данные:

- Indicator correction - подстройка стрелочного индикатора

## Configuration data:

- Indicator correction - adjustment of the arrow indicator

  ![EFEKTA Open PM Monitor](https://raw.githubusercontent.com/smartboxchannel/EFEKTA-Open_PM_Monitor/refs/heads/main/IMAGES/image%20(5).png)

## Ввод датчика в сеть, выход из сети

Для джойна (вход в сеть) включите джойн, зажмите и удерживайте кнопку сбоку датчика. С задней стороны датчика загорится системный светодиод (примерно на 3-5 секунд при успешном поиске открытой сети, 15 секунд при неудачном поиске открытой сети).

Если вы не видите на вкладке "Свойства" всех значений конфигурационных атрибутов (пустые поля, переключатели в неопределённом состоянии) или на вкладке "Отчёты" нет заполненных строк о типах данных PM и т.д., то скорее всего конфигурация, которая следует сразу за интервью, не была пройдена до конца.

Для повторного прохождения конфигурации перейдите на главную страницу z2m, найдите строку датчика и справа нажмите на кнопку "Реконфигурация" (жёлтая кнопка), после этого несколько раз нажмите на кнопку джойстик сбоку - это вызовет отправку всех основных и конфигурационных данных. При успешном прохождении конфигурации в интерфейсе z2m должно появиться всплывающее сообщение об успешно пройденной реконфигурации. В разделе датчика на вкладке "Свойства" должны появиться значения и установки всех конфигурационных свойств, на странице "Отчёты" должны появиться строки с конфигурационными настройками отчётов.


Когда датчик в сети, то короткое нажатие на кнопку вызывает процедуру чтения всех сенсоров не по расписанию и отправку данных не в режиме настроенных отчётов.

Для выхода из сети зажмите кнопку на 10 секунд. С задней стороны датчика начнёт мигать системный светодиод (частота переключения LED - 1 секунда). Когда светодиод перестанет мигать, кнопку можно отпустить. Датчик отправит сообщение о выходе из сети, сотрёт у себя все настройки в памяти.

Также выйти из сети можно, удалив датчик из z2m без опции "force remove".

## Entering the sensor into the network, leaving the network

For joining (entering the network), turn on the join, press and hold the button on the side of the sensor. The system LED on the back of the sensor will light up (for about 3-5 seconds if the search for an open network is successful, 15 seconds if the search for an open network is unsuccessful).

If you do not see all the values ​​of the configuration attributes on the "Properties" tab (empty fields, switches in an undefined state) or there are no filled lines on the "Reports" tab about the PM data types, etc., then most likely the configuration that immediately follows the interview was not completed.

To repeat the configuration, go to the main page of z2m, find the sensor line and click on the "Reconfiguration" button on the right (yellow button), then press the joystick button on the side several times - this will cause all the main and configuration data to be sent. If the configuration is successful, a pop-up message about the successful reconfiguration should appear in the z2m interface. In the sensor section, on the "Properties" tab, the values ​​and settings of all configuration properties should appear, and on the "Reports" page, lines with the configuration settings of reports should appear.

When the sensor is online, a short press on the button causes the procedure of reading all sensors not according to the schedule and sending data not in the configured reports mode.

To exit the network, hold the button for 10 seconds. The system LED on the back of the sensor will start blinking (LED switching frequency is 1 second). When the LED stops blinking, you can release the button. The sensor will send a message about leaving the network, erasing all settings in its memory.

You can also exit the network by deleting the sensor from z2m without the "force remove" option.


# Цветовая индикация 

## Индекс качества воздуха на основе PM2.5:

- < 120 - зелёный
- ≥120 < 250 - жёлтый
- ≥250 < 380 - розовый
- ≥380 - красный

## Air quality index based on PM2.5:

- < 120 - green
- ≥120 < 250 - yellow
- ≥250 < 380 - pink
- ≥380 - red

  
