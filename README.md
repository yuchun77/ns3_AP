# ns3_AP

使用 Network Simulator(ns-3) 模擬兩 WLAN 之間的封包傳輸。

* 2 WLAN 中包含：
  * 2 AP
  * 3 Nodes
* 使用 IPv6 作為 IP address
* 使用 UDP Protocol
  * 每個 AP 可以和其底下的 STA 聯絡
  * 所有節點使用 UDP 溝通
* 模擬網路架構如下圖：
  <p align="left">
    <img src="https://github.com/user-attachments/assets/3793fbbe-0e04-48b6-a1ce-24a14ec36e4e" width="60%">
  </p>

詳細 code 請看`project1.cc`
