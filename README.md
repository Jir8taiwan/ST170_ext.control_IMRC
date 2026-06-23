# ST170_ext.control_IMRC
**Ford Focus MK1(ST170) external control IMRC via Arduino Hall sensor of tachometer**

**Ford Focus MK1(ST170) Modified IMRC and drive motor by external CC/CV power module**

**Ford Focus MK1(ST170) Delete function and bypass IMRC module**

Original story is in my blog, but I also maintain in GitHub to share my code and usage ideas.
https://jir.idv.tw/wordpress/?p=2745

If this small code is helping through Arduino kits, it can donate BCH coin to me for encourage as following address:
1. BTC - 3M4wWghm4MxmrSfXmHMEeCFNwP8Lxxqjzk
2. BCH - bitcoincash:qq6ghvdmyusnse9735rd5q09ensacl8z8qzrlwf49q
3. LTC - MR6HaFkfkmsfifX3jWu7xz33dULGotVUWB
4. DOGE- DGEFd3AAfJrBuaUwc4P6R2ZT754Jon9fQ7

Thank you very much.


**** Copy from my site:
[DIY]福特ST170的IMRC模組，外掛Arduino+霍爾感應器抓引擎轉速驅動、或外掛恆流電源模組直接驅動馬達
2020-12-27 作者: Jir
*** 編輯更新：2026.06.15 ***



前言，我這台ST170的IMRC模組，雖然因為跳了P1518後換了一顆新的不會再出現警報。
但是後來有明確發現，在引擎啟動時，都沒有做正常的開啟(長通道循環)的運作，造成起步和加速相對無力、以及耗油問題。
看電路圖，猜他可能線組的中繼接頭應該是有接觸不良才造成這樣，但是我也懶得拆在東拆西查修了。
乾脆直接用外掛監控和驅動的方式去控制IMRC模組何時用短通道和長通道。
基本要控制IMRC模組的地方，就是接頭的PIN1(GND)和PIN3(SIGNAL CONTRL)腳。
當訊號接地，他就會做動成長通道模式，適用6000RPM以下的低速扭力需求。
當訊號斷路，他則會恢復放鬆成短通道模式，適用6000-8500RPM的高速吸氣需求。
如果不想要做動，只是要強制ACC後固定長通道，也可以拿一個二極體直接兩腳位導通就好。

NOTE:
1. ACC紅火啟動，PCM會使PIN3導通接地，IMRC模組於1.5s內要拉鋼索，把進氣岐管轉到長通道(扭力模式)。
2. IMRC模組鋼索拉動後，裡面的銅片金屬接點結合，觸發電阻75R迴路給PIN5和PIN6，告知PCM鋼索機構沒有卡住。



(1) 方案一，外部感應轉速訊號，強制IMRC模組不經過PCM控制，導通拉動／放鬆鋼索
需求材料：
Arduino NANO板
HALL感應器模組
1路5V RELAY模組
12V轉5V電源轉換模組
PVC絞線0.5或0.3mm^2，四心線
腳位纜線

配線：
IMRC模組的PIN1(GND)、PIN2(VCC)、PIN3(CONTROL)；12V正電和接地接到12V轉5V模組；控制線和接地則分別接到RELAY模組的NC和COM。
HALL感應器模組的正極接3.3V、負極接GND、數位訊號接7號腳。
RELAY模組的正極接12號腳，負極接GND。
12V轉5V模組的USB輸出線，接Arduino NANO板的USB電源線


工作判斷設計原理：
霍爾感應器回饋訊號給7號腳，計算比對當下的引擎轉速值。
判斷當3900RPM於0.5秒後，又超過4000RPM時，則12號腳輸出給RELAY，讓本來PIN1和PIN3短路狀態變斷路。

過程中，發現RELAY會亂做動。
用Serial埠見控數值後，怠速下回饋的RPM轉速訊號計算值會破9000~12000RPM左右的誤差值~XD
看起來我安裝SENSOR的位置，有極大的雜訊會造成干擾誤算。
所以先下一個除以12倍的方式做初步的濾波計算，看起來就能相對正常計算繼電器的啟閉時機了。

等有空一點，改成監控點火電晶體的12V訊號好了。
這個應該不會有周遭電場的雜訊干擾，只是要怎麼監控訊號，又不干擾點火系統的高阻抗電路，我要再想想。

利用EasyEDA軟體製作成品，PDF原始檔連結：
[planA]_ArduinoIMRC_DCmotorControl\ArduinoIMRC_DCmotorControl_2025-12-08.pdf

最新程式碼同步更新在GitHub那裏：https://github.com/Jir8taiwan/ST170_ext.control_IMRC
程式碼：
[planA]_ArduinoIMRC_DCmotorControl\



(2) 方案二，保持PCM控制訊號，IMRC模組內部迴路修改，由外掛DC馬達驅動器，導通拉動／放鬆鋼索

修改控制配線圖：
[planB]_IMRCmod_and_DirectDrive_Motor\Schematic_IMRCmod_DirectDrive_Motor_2026-06-17.pdf


IMRC模組需內部修改直接馬達控制供電(若已故障的Q1 TIP121電晶體可以順道移除，使之斷路)：
[planB]_IMRCmod_and_DirectDrive_Motor\IMRC internal modify_2_2026-06-15.jpg


材料：
1. Mini560 Pro 固定5V電源模組
2. 5V Relay 高低電位繼電器模組
3. FP5139 (SJ4) 自動升降壓CC CV模組 (設定12V、1.5A)
或 LM2596 壓降型CC CV模組 (設定1.4V、1.5A、以及加強散熱片)
4. A1015 PNP電晶體 *1
20K 電阻 *1
2K 電阻 *1
5. 6P 車用公插頭、母插座 (拿TOYOTA車種修改)
6. 6C或4C+2C 0.75mm(1.25mm)電纜線
7. [選配 – 看75R接合回饋狀態]：
2SC1815 NPN電晶體 *1
33K 電阻 *1 (或47K)
2K 電阻 *1
LED 發光二極體 *1
8. [選配 – CC/CV電源模組加強穩壓抗干擾用]
1000uF 50V 電解電容 *2
103(0.1uF) 陶瓷電容 *2

工作判斷設計原理：
PCM在PIN1 CTRL腳位，會從5V拉低到4V要求IMRC工作，馬達要做動變成長通道模式。
透過A1015電晶體開關迴路，監控此電壓值變化，讓5V繼電器模組可以高電位工作。
5V繼電器導通電壓源給馬達工作，電壓源需要恆壓恆流模擬原始IMRC模組阻轉的工作參數。
FP5139自動升降壓，設定12V供應固定電壓源，然後最大只到1.5A電流給馬達定位後阻轉。
阻轉狀態，內部的訊號回饋電阻75R會被IMRC內部的轉盤機械結構接合處發PIN5 MON和PIN6 RTN迴路，使PCM知道目前已經是長通道狀態。



(3) 方案三，保持PCM控制訊號，IMRC模組整組改成客製全模組直流驅動馬達轉動鋼索機構
(專案計畫建構中)

其他參考文章：
http://stm32-learning.blogspot.com/2014/05/arduino.html
http://59.126.75.42/blog/blog.php?uid=shadow&id=1862
https://makersportal.com/blog/2018/10/3/arduino-tachometer-using-a-hall-effect-sensor-to-measure-rotations-from-a-fan
https://kokoraskostas.blogspot.com/2013/12/arduino-inductive-spark-plug-sensor.html
https://www.scribd.com/document/758299998/Manual-IMRC
