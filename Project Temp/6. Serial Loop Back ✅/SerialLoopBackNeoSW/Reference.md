https://forum.arduino.cc/t/should-i-be-able-to-perform-a-loopback-test-with-softwareserial/209185/1
https://forum.arduino.cc/t/softwareserial-library-isnt-reliable-in-a-loopback-sketch-even-if-2-bytes-sent/377705
https://github.com/PaulStoffregen/AltSoftSeria
https://github.com/SlashDevin/NeoSWSerial

You're encountering an issue that often arises due to the nature of SoftwareSerial and how it handles timing. Let me break it down:
Why This Happens, Interrupt Limitations of SoftwareSerial. SoftwareSerial is heavily dependent on interrupts to simulate a UART (serial) interface.
When you're trying to read and write in quick succession on the same software serial instance, it can miss data due to timing conflicts. This is especially true for the "echo" type behavior in your code.
No Hardware Buffer, Unlike HardwareSerial (e.g., Serial), SoftwareSerial has no hardware buffer, so if data is missed during an interrupt, it's lost forever.
Loopback Issue, When you connect TX (pin 10) to RX (pin 11), the timing of the write() and read() calls might not align perfectly because SoftwareSerial processes data sequentially. If a write() occurs, but no read() is immediately ready, data is lost.

** Use a Hardware UART Multiplexer **
If you must stick to software serial for both TX and RX on the same Arduino, consider using a multiplexing library such as AltSoftSerial or NeoSWSerial. These libraries are often more efficient and avoid some of the pitfalls of SoftwareSerial.

** Kesimpulan **
Penyebab utama masalah dengan SoftwareSerial:

Konflik interrupt karena operasi TX dan RX simultan.
Ketergantungan pada timer yang sudah digunakan oleh sistem lain.
Emulasi UART yang membutuhkan banyak waktu CPU, terutama dalam pengaturan loopback.
AltSoftSerial mengatasi masalah ini dengan:

Menggunakan timer khusus (Timer 1) yang tidak terganggu oleh operasi sistem lainnya.
Pengelolaan interrupt yang lebih efisien dan buffer yang lebih andal.
Rekomendasi
Gunakan AltSoftSerial jika memungkinkan, terutama untuk aplikasi di mana reliabilitas komunikasi serial sangat penting.
Hindari penggunaan SoftwareSerial untuk pengaturan loopback atau aplikasi yang membutuhkan RX dan TX simultan pada perangkat yang sama.