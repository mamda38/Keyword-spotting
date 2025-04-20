# 🔌Hệ thống nhận diện giọng nói trên thiết bị nhúng ESP32S3 sử dụng TinyML 

---

## 📑 Mục Lục

- [Giới thiệu](#giới-thiệu)
- [Danh sách linh kiện](#danh-sách-linh-kiện)
- [Sơ đồ nguyên lý](#sơ-đồ-nguyên-lý)
- [Lập trình](#lập-trình)
- [Demo](#demo)

---

## Giới thiệu

📌 Dự án: Phát triển hệ thống nhận diện từ khóa giọng nói trên vi điều khiển Xiao ESP32S3 bằng mô hình học máy (Edge Impulse). Khi người dùng nói các câu lệnh như "bật đèn", "tắt quạt", hệ thống sẽ nhận diện và điều khiển thiết bị tương ứng (LED, quạt, v.v).

🎯 Mục đích: Tạo ra thiết bị điều khiển thông minh bằng giọng nói, ứng dụng cho nhà thông minh, thiết bị IoT hoặc giao tiếp không chạm trong môi trường thực tế.

---


## 🧰 Danh Sách Linh Kiện

| Tên linh kiện                       | Số lượng | 
|-------------------------------------|----------|
| Seeed Studio XIAO ESP32S3 Sense     | 1        | 
| LED Diode                           | 2        | 
| Điện trở 1kΩ                        | 2        | 


---

## 🔧 Sơ Đồ Nguyên Lý

- 📎 [Schematic (PDF)](docs/schematic.pdf)
- 📎 [PCB Layout (Gerber)](docs/gerber.zip)
- 📎 [File thiết kế (Eagle / KiCad)](docs/project.kicad_pcb)

_Hình minh họa sơ đồ nguyên lý hoặc board PCB có thể nhúng ngay tại đây:_

![Schematic](docs/images/schematic.png)


---

## 💻 Lập Trình 

- **Ngôn ngữ:** C++ (Arduino IDE) 
- **Verify:**
  ```bash
  Phím tắt: Ctrl + R
- **Upload:**
  ```bash
  Phím tắt: Ctrl + U
- **Serial Monitor:**
  ```bash
  Phím tắt: Ctrl + Shift + M

---


## Video Demo


[![Xem video demo](https://img.youtube.com/vi/V25H6bmhsMc/0.jpg)](https://youtu.be/V25H6bmhsMc)
