# ESLAB_hw7

### Main Functions
* 透過 STM32L4 偵測 GYPO 和 ACCELERO，並將訊號通過 FIR low pass filter以去除雜訊。
* 經由 MATLAB 繪出初始訊號和去雜訊的訊號。

### How to run

##### STM32L4
* 可以直接將整個project載下，用 mbed studio 將程式燒錄在 STM32L4 開發板上。

##### MATLAB
* 由sample.m 和 sensor.m 分別畫出 figure1 和 figure2, 3.
* 可替換 sensor.m 中的 data 為每次 STM32L4 偵測與過濾後輸出的結果。

###### figure1
  ![image](https://user-images.githubusercontent.com/71332212/144066670-0a1dbcd6-9ac9-4dd6-a173-beb986bf4103.png)

###### figure2,3
  ![image](https://user-images.githubusercontent.com/71332212/144066694-e3e0c225-5005-4bba-bcf8-a96bcd8f7472.png)
  ![image](https://user-images.githubusercontent.com/71332212/144066725-cda6b853-9992-4e82-bded-a788eb175b8d.png)

* figure1是sample data在波形在時域和頻域上作圖。
* figure2是實際 GYPO 的 testing data 在時域上做圖。
* figure3是實際 GYPO 的 testing data 在頻域上做圖。
* figure中左列是過 FIR low pass filter 前，右列是過 filter 後的結果。


### Demo Result

```
Getting data...

Testing Correctness
SNR THRESHOLD: 75.000000
input: 0.000000,  ref: 0.000000,  test: 0.000000
input: 0.592466,  ref: -0.001080,  test: -0.001080
input: -0.094734,  ref: -0.000768,  test: -0.000768
...
snr_threshold: 75.000000. snr: 142.585007
SUCCESS
GYRO_X = [ -210.000000 70.000000 -140.000000 ...]
GYRO_Y = [ -3640.000000 -2800.000000 -2940.000000 ...]
GYRO_Z = [ 700.000000 630.000000 560.000000 ...]
ACC_X = [ 273.000000 271.000000 271.000000 ...]
ACC_Y = [ -139.000000 -140.000000 -140.000000 ...]
ACC_Z = [ 978.000000 974.000000 975.000000 ...]
GYRO_OUTPUT_X = [ 0.382730 0.205889 0.143998 ...]
GYRO_OUTPUT_Y = [ -148.381744 -175.138779 -196.881577 ...]
GYRO_OUTPUT_Z = [ -3036.771240 -3011.972656 -2966.527588 ...]
ACC_OUTPUT_X = [ 501.441650 444.582397 421.799744 ...]
ACC_OUTPUT_Y = [ 272.289398 273.849426 275.248810 ...]
ACC_OUTPUT_Z = [ -143.947449 -148.009933 -151.203140 ...]
```

* 最初，我們會偵測 GYPO 和 ACCELERO data
* 接著，藉由已知的 sample data 驗證我們 FIR low pass filter 的正確性
* 最後，輸出偵測到的 GYPO 和 ACCELERO data，和過 filter 後的結果
