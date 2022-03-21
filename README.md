# ST170_ext.control_IMRC
Ford Focus MK1(ST170) external control IMRC via Arduino Hall sensor of tachometer

Original story and maintain in GitHub
https://jir.idv.tw/wordpress/?p=2745

[DIY]福特ST170的IMRC模組，外掛Arduino+霍爾感應器抓引擎轉速驅動
發佈日期: 2020-12-27，作者: Jir


前言，我這台ST170的IMRC模組，雖然因為跳了P1518後換了一顆新的不會再出現警報。
但是後來有明確發現，在引擎啟動時，都沒有做正常的開啟(長通道循環)的運作，造成起步和加速相對無力、以及耗油問題。
看電路圖，猜他可能線組的中繼接頭應該是有接觸不良才造成這樣，但是我也懶得拆在東拆西查修了。
乾脆直接用外掛監控和驅動的方式去控制IMRC模組何時用短通道和長通道。
基本要控制IMRC模組的地方，就是接頭的PIN1(GND)和PIN3(SIGNAL CONTRL)腳。
當訊號接地，他就會做動成長通道模式，適用6000RPM以下的低速扭力需求。
當訊號斷路，他則會恢復放鬆成短通道模式，適用6000-8500RPM的高速吸氣需求。
如果不想要做動，只是要強制ACC後固定長通道，也可以拿一個二極體直接兩腳位導通就好。




需求材料：
Arduino NANO板
HALL感應器模組
1路5V RELAY模組
12V轉5V電源轉換模組
電話用四心線
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



參考文章：
http://stm32-learning.blogspot.com/2014/05/arduino.html

http://59.126.75.42/blog/blog.php?uid=shadow&id=1862

https://makersportal.com/blog/2018/10/3/arduino-tachometer-using-a-hall-effect-sensor-to-measure-rotations-from-a-fan

https://kokoraskostas.blogspot.com/2013/12/arduino-inductive-spark-plug-sensor.html
