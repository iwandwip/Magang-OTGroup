# Dokumentasi Sistem PalletizerMaster

## Daftar Isi
1. [Pendahuluan](#1-pendahuluan)
2. [Perintah Dasar Sistem](#2-perintah-dasar-sistem)
3. [Perintah Kontrol Gerakan](#3-perintah-kontrol-gerakan)
4. [ID Slave dan Fungsinya](#4-id-slave-dan-fungsinya)
5. [Format Parameter](#5-format-parameter)
6. [Sistem Antrian Perintah](#6-sistem-antrian-perintah)
7. [Status Sistem dan Indikator](#7-status-sistem-dan-indikator)
8. [Komunikasi Internal](#8-komunikasi-internal)
9. [Alur Kerja Eksekusi Perintah](#9-alur-kerja-eksekusi-perintah)
10. [Contoh Penggunaan](#10-contoh-penggunaan)

## 1. Pendahuluan

Dokumen ini berisi instruksi penggunaan dan spesifikasi teknis untuk sistem PalletizerMaster. Sistem ini dirancang untuk mengontrol gerakan motor pada beberapa sumbu (X, Y, Z, T, dan Gripper) melalui komunikasi serial dan antarmuka Bluetooth.

PalletizerMaster bertindak sebagai koordinator pusat yang menerima perintah dari perangkat Android, memproses instruksi tersebut, dan mendistribusikannya ke unit slave yang sesuai. Sistem ini mendukung pemrosesan antrian perintah dan berbagai status operasi.

## 2. Perintah Dasar Sistem

Berikut adalah perintah-perintah dasar untuk mengontrol status operasi sistem:

### 2.1 Perintah Status Sistem

| Perintah | Deskripsi |
|----------|-----------|
| `IDLE` | Menghentikan semua operasi dan mengatur sistem ke status idle. Jika sistem sedang menjalankan sebuah urutan, sistem akan memasuki status STOPPING hingga urutan selesai. |
| `PLAY` | Memulai atau melanjutkan eksekusi. Jika ada perintah dalam antrian, sistem akan langsung memprosesnya. |
| `PAUSE` | Menunda eksekusi. Sistem akan menyelesaikan perintah saat ini tetapi tidak akan memproses perintah berikutnya dari antrian. |
| `STOP` | Menghentikan semua operasi. Jika sistem sedang menjalankan urutan, sistem akan menyelesaikan urutan saat ini terlebih dahulu. |

### 2.2 Cara Penggunaan

Perintah status sistem digunakan sebagai perintah mandiri, tanpa parameter tambahan.

**Contoh:**
```
PLAY
PAUSE
STOP
IDLE
```

## 3. Perintah Kontrol Gerakan

Perintah kontrol gerakan digunakan untuk menggerakkan sumbu dan mengatur kecepatan.

### 3.1 Perintah ZERO

Perintah `ZERO` mengembalikan semua sumbu ke posisi awal/home.

**Format:** `ZERO`

**Contoh:** `ZERO`

### 3.2 Perintah Koordinat

Perintah koordinat menggerakkan sumbu-sumbu ke posisi tertentu.

**Format:** `slaveid(param1,param2), slaveid(param1,param2), ...`

Dalam format ini:
- `slaveid` adalah identifikasi slave (x, y, z, t, g)
- `param1` adalah posisi target
- `param2` adalah kecepatan gerakan (opsional)

**Contoh:**
- `x(100,500)` - Menggerakkan sumbu X ke posisi 100 dengan kecepatan 500
- `y(200)` - Menggerakkan sumbu Y ke posisi 200 dengan kecepatan default
- `x(100,500), y(200,500), z(300)` - Menggerakkan beberapa sumbu dengan satu perintah

### 3.3 Perintah SPEED

Perintah `SPEED` mengatur kecepatan untuk sumbu tertentu atau semua sumbu.

**Format:**
- Untuk satu sumbu: `SPEED;slaveid;value`
- Untuk semua sumbu: `SPEED;value`

**Contoh:**
- `SPEED;x;500` - Mengatur kecepatan sumbu X ke 500
- `SPEED;200` - Mengatur kecepatan semua sumbu ke 200

## 4. ID Slave dan Fungsinya

Sistem PalletizerMaster mendukung beberapa slave untuk mengontrol berbagai sumbu gerakan.

### 4.1 Daftar ID Slave

| ID Slave | Deskripsi | Format Perintah |
|----------|-----------|-----------------|
| **x** | Sumbu X (horizontal) | `x(posisi,kecepatan)` |
| **y** | Sumbu Y (vertikal) | `y(posisi,kecepatan)` |
| **z** | Sumbu Z (kedalaman) | `z(posisi,kecepatan)` |
| **t** | Sumbu T (rotasi) | `t(posisi,kecepatan)` |
| **g** | Gripper (penjepit) | `g(posisi)` |

### 4.2 Fungsi Masing-masing Slave

- **Sumbu X:** Mengontrol gerakan horizontal (kiri-kanan)
- **Sumbu Y:** Mengontrol gerakan vertikal (atas-bawah)
- **Sumbu Z:** Mengontrol gerakan kedalaman (depan-belakang)
- **Sumbu T:** Mengontrol gerakan rotasi
- **Gripper:** Mengontrol mekanisme penjepit (buka-tutup)

## 5. Format Parameter

### 5.1 Parameter untuk Perintah Koordinat

Perintah koordinat menggunakan format: `slaveid(param1,param2,...)`

| Parameter | Deskripsi | Satuan | Wajib? |
|-----------|-----------|--------|--------|
| **param1** | Posisi target | Step/Pulsa | Ya |
| **param2** | Kecepatan gerakan | Step/detik | Tidak |

**Catatan:** Jika parameter kecepatan tidak ditentukan, sistem akan menggunakan kecepatan default atau kecepatan yang diatur sebelumnya.

### 5.2 Parameter untuk Perintah SPEED

| Format | Deskripsi | Contoh |
|--------|-----------|--------|
| `SPEED;slaveid;value` | Mengatur kecepatan untuk satu slave | `SPEED;x;500` |
| `SPEED;value` | Mengatur kecepatan untuk semua slave | `SPEED;200` |

## 6. Sistem Antrian Perintah

PalletizerMaster mengimplementasikan sistem antrian untuk memproses beberapa perintah secara berurutan.

### 6.1 Karakteristik Antrian

- **Kapasitas:** Maksimum 5 perintah
- **Metode:** First-In-First-Out (FIFO)
- **Status:** Antrian hanya aktif diproses saat sistem dalam status RUNNING

### 6.2 Perintah Terkait Antrian

| Perintah | Deskripsi | Contoh |
|----------|-----------|--------|
| `END_QUEUE` | Menandakan akhir dari antrian perintah | `END_QUEUE` |
| `NEXT` | Dikirim oleh master ke Android untuk meminta perintah berikutnya (internal) | - |
| `DONE` | Dikirim oleh master ke Android untuk menandakan bahwa urutan telah selesai (internal) | - |

### 6.3 Cara Kerja Antrian

1. Perintah ditambahkan ke antrian saat sistem sibuk atau dalam status PAUSED
2. Sistem akan mengirim permintaan "NEXT" ke Android ketika antrian hampir kosong
3. Perintah diambil dari antrian dan dieksekusi satu per satu
4. Setelah menyelesaikan suatu perintah, sistem mengirim "DONE" ke Android
5. Sistem akan melanjutkan ke perintah berikutnya dalam antrian jika berada dalam status RUNNING

## 7. Status Sistem dan Indikator

### 7.1 Status Sistem

| Status | Kode | Deskripsi |
|--------|------|-----------|
| **STATE_IDLE** | 0 | Sistem dalam keadaan siap/diam |
| **STATE_RUNNING** | 1 | Sistem sedang menjalankan perintah |
| **STATE_PAUSED** | 2 | Sistem dijeda |
| **STATE_STOPPING** | 3 | Sistem sedang dalam proses berhenti |

### 7.2 Indikator LED

| Status | Indikator LED |
|--------|---------------|
| STATE_IDLE | Merah |
| STATE_RUNNING | Hijau |
| STATE_PAUSED | Kuning |
| STATE_STOPPING | Merah |

### 7.3 Perubahan Status

Status sistem dapat berubah berdasarkan perintah yang diterima atau hasil dari eksekusi perintah:
- `IDLE` → Sistem beralih ke STATE_IDLE
- `PLAY` → Sistem beralih ke STATE_RUNNING
- `PAUSE` → Sistem beralih ke STATE_PAUSED
- `STOP` → Sistem beralih ke STATE_STOPPING, kemudian ke STATE_IDLE
- Setelah semua perintah selesai → Beralih ke STATE_IDLE

## 8. Komunikasi Internal

### 8.1 Format Perintah ke Slave

Perintah dikirim dari master ke slave dengan format:

`slaveid;command_code;parameters`

Dimana:
- `slaveid` adalah identifikasi slave (x, y, z, t, g)
- `command_code` adalah nilai numerik enum Command
- `parameters` adalah parameter tambahan, dipisahkan oleh titik koma

### 8.2 Kode Command Internal

| Command | Kode | Deskripsi | Format |
|---------|------|-----------|--------|
| CMD_NONE | 0 | Tidak ada perintah | - |
| CMD_RUN | 1 | Menjalankan motor ke posisi tertentu | `slaveid;1;posisi;kecepatan` |
| CMD_ZERO | 2 | Mengembalikan motor ke posisi awal | `slaveid;2` |
| CMD_SETSPEED | 6 | Mengatur kecepatan motor | `slaveid;6;kecepatan` |

### 8.3 Pesan Respons dari Slave

Slave dapat mengirim pesan respons ke master:
- `SEQUENCE COMPLETED` - Menandakan urutan telah selesai

## 9. Alur Kerja Eksekusi Perintah

### 9.1 Inisialisasi

1. Sistem memulai dalam status STATE_IDLE
2. LED indikator merah menyala

### 9.2 Pengaktifan

1. Pengguna mengirim perintah `PLAY`
2. Sistem beralih ke STATE_RUNNING
3. LED indikator hijau menyala
4. Sistem mulai memproses perintah dari antrian jika ada

### 9.3 Eksekusi Perintah

1. Sistem mengambil perintah pertama dari antrian
2. Perintah diproses dan dikirimkan ke slave yang sesuai
3. Flag `sequenceRunning` diatur ke true
4. Jika `indicatorEnabled` = true, sistem akan memeriksa pin indikator untuk mengetahui penyelesaian
5. Jika `indicatorEnabled` = false, sistem akan menunggu pesan "SEQUENCE COMPLETED" dari slave

### 9.4 Penyelesaian Perintah

1. Ketika perintah selesai, sistem mengirim "DONE" ke Android
2. Jika masih ada perintah dalam antrian dan status = STATE_RUNNING, sistem akan memproses perintah berikutnya
3. Jika antrian kosong dan status = STATE_RUNNING, sistem beralih ke STATE_IDLE

### 9.5 Penghentian

1. Saat menerima perintah `PAUSE`, sistem menyelesaikan perintah saat ini tetapi tidak memproses perintah berikutnya
2. Saat menerima perintah `STOP` atau `IDLE`, sistem beralih ke STATE_STOPPING
3. Setelah perintah saat ini selesai, sistem membersihkan antrian dan beralih ke STATE_IDLE

## 10. Contoh Penggunaan

### 10.1 Contoh Sekuens Perintah Sederhana

```
PLAY
ZERO
x(1000,500)
y(1000,500)
STOP
```

Urutan ini akan:
1. Mengaktifkan sistem ke STATE_RUNNING
2. Mengembalikan semua sumbu ke posisi awal
3. Menggerakkan sumbu X ke posisi 1000 dengan kecepatan 500
4. Menggerakkan sumbu Y ke posisi 1000 dengan kecepatan 500
5. Menghentikan sistem

### 10.2 Contoh Sekuens Perintah Lengkap

```
PLAY
ZERO
SPEED;500
x(1000,500), y(1000,500)
z(500,300)
g(1)
x(0,500), y(0,500), z(0,300)
STOP
```

Urutan ini akan:
1. Mengaktifkan sistem ke STATE_RUNNING
2. Mengembalikan semua sumbu ke posisi awal
3. Mengatur kecepatan default 500 untuk semua sumbu
4. Menggerakkan sumbu X dan Y ke posisi 1000
5. Menggerakkan sumbu Z ke posisi 500
6. Mengaktifkan gripper
7. Mengembalikan semua sumbu ke posisi 0
8. Menghentikan sistem

### 10.3 Penggunaan dengan Antrian

```
PLAY
ZERO
x(1000,500)
PAUSE
y(1000,500)
PLAY
z(500,300)
END_QUEUE
```

Urutan ini akan:
1. Memulai eksekusi
2. Mengembalikan semua sumbu ke posisi awal
3. Menggerakkan sumbu X ke posisi 1000
4. Menjeda eksekusi (sistem tidak akan memproses perintah y(1000,500) sampai dimainkan kembali)
5. Menambahkan perintah y(1000,500) ke antrian
6. Melanjutkan eksekusi (memproses perintah y(1000,500))
7. Menambahkan perintah z(500,300) ke antrian
8. Menandai akhir dari antrian perintah