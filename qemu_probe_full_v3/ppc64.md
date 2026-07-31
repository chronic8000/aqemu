# QEMU Complete Architecture capability map: `ppc64`

- **Architecture Target:** `ppc64`
- **Binary Executable:** `C:\Users\chron\CURSOR-PROJECTS\aqemu\build_win\qemu-system-ppc64.exe`
- **Probed At:** 2026-07-31T08:33:04.516750
- **Fallback Machine Used for Context:** `pseries`

> **INSTRUCTIONS FOR CURSOR AI:** This document contains the verified whitelist of supported flags, boards, CPUs, devices, storage drivers, audio backends, and display renderers for `qemu-system-{arch}`. Use this data as the absolute ground truth to construct and validate VM configuration parameters.

## 1. Supported Machines (`-machine help`)

```text
Supported machines are:
40p                  IBM RS/6000 7020 (40p)
amigaone             Eyetech AmigaOne/Mai Logic Teron
bamboo               bamboo
g3beige              Heathrow based PowerMac
mac99                Mac99 based PowerMac
mpc8544ds            mpc8544ds
none                 empty machine
pegasos1             Genesi/bPlan Pegasos I
pegasos2             Genesi/bPlan Pegasos II
powernv              IBM PowerNV (Non-Virtualized) POWER10 (alias of powernv10)
powernv10            IBM PowerNV (Non-Virtualized) POWER10
powernv              IBM PowerNV (Non-Virtualized) POWER10 Rainier (alias of powernv10-rainier)
powernv10-rainier    IBM PowerNV (Non-Virtualized) POWER10 Rainier
powernv11            IBM PowerNV (Non-Virtualized) Power11
powernv8             IBM PowerNV (Non-Virtualized) POWER8
powernv9             IBM PowerNV (Non-Virtualized) POWER9
ppce500              generic paravirt e500 platform
ppe42_machine        PPE42 Test Machine
pseries-10.0         pSeries Logical Partition (PAPR compliant)
pseries-10.1         pSeries Logical Partition (PAPR compliant)
pseries-10.2         pSeries Logical Partition (PAPR compliant)
pseries              pSeries Logical Partition (PAPR compliant) (alias of pseries-11.0)
pseries-11.0         pSeries Logical Partition (PAPR compliant) (default)
pseries-5.1          pSeries Logical Partition (PAPR compliant) (deprecated)
pseries-5.2          pSeries Logical Partition (PAPR compliant) (deprecated)
pseries-6.0          pSeries Logical Partition (PAPR compliant) (deprecated)
pseries-6.1          pSeries Logical Partition (PAPR compliant) (deprecated)
pseries-6.2          pSeries Logical Partition (PAPR compliant) (deprecated)
pseries-7.0          pSeries Logical Partition (PAPR compliant) (deprecated)
pseries-7.1          pSeries Logical Partition (PAPR compliant) (deprecated)
pseries-7.2          pSeries Logical Partition (PAPR compliant) (deprecated)
pseries-8.0          pSeries Logical Partition (PAPR compliant) (deprecated)
pseries-8.1          pSeries Logical Partition (PAPR compliant)
pseries-8.2          pSeries Logical Partition (PAPR compliant)
pseries-9.0          pSeries Logical Partition (PAPR compliant)
pseries-9.1          pSeries Logical Partition (PAPR compliant)
pseries-9.2          pSeries Logical Partition (PAPR compliant)
sam460ex             aCube Sam460ex
virtex-ml507         Xilinx Virtex ML507 reference design
```
Total items listed: 40

## 2. Supported CPUs (`-cpu help`)

```text
Available CPUs:
  603              PVR 00030100
  mpc8240          (alias for 603)
  vanilla          (alias for 603)
  604              PVR 00040103
  ppc32            (alias for 604)
  ppc              (alias for 604)
  603e_v1.1        PVR 00060101
  603e_v1.2        PVR 00060102
  603e_v1.3        PVR 00060103
  603e_v1.4        PVR 00060104
  603e_v2.2        PVR 00060202
  603e_v3          PVR 00060300
  603e_v4          PVR 00060400
  603e_v4.1        PVR 00060401
  603e             (alias for 603e_v4.1)
  stretch          (alias for 603e_v4.1)
  603p             PVR 00070000
  603e7v           PVR 00070100
  vaillant         (alias for 603e7v)
  603e7v1          PVR 00070101
  603e7            PVR 00070200
  603e7v2          PVR 00070201
  603e7t           PVR 00071201
  603r             (alias for 603e7t)
  goldeneye        (alias for 603e7t)
  750_v1.0         PVR 00080100
  740_v1.0         PVR 00080100
  740e             PVR 00080100
  750e             PVR 00080200
  750_v2.0         PVR 00080200
  740_v2.0         PVR 00080200
  750_v2.1         PVR 00080201
  740_v2.1         PVR 00080201
  750_v2.2         PVR 00080202
  740_v2.2         PVR 00080202
  750_v3.0         PVR 00080300
  740_v3.0         PVR 00080300
  750_v3.1         PVR 00080301
  750              (alias for 750_v3.1)
  typhoon          (alias for 750_v3.1)
  g3               (alias for 750_v3.1)
  740_v3.1         PVR 00080301
  740              (alias for 740_v3.1)
  arthur           (alias for 740_v3.1)
  750cx_v1.0       PVR 00082100
  750cx_v2.0       PVR 00082200
  750cx_v2.1       PVR 00082201
  750cx_v2.2       PVR 00082202
  750cx            (alias for 750cx_v2.2)
  750cxe_v2.1      PVR 00082211
  750cxe_v2.2      PVR 00082212
  750cxe_v2.3      PVR 00082213
  750cxe_v2.4      PVR 00082214
  750cxe_v3.0      PVR 00082310
  750cxe_v3.1      PVR 00082311
  745_v1.0         PVR 00083100
  755_v1.0         PVR 00083100
  745_v1.1         PVR 00083101
  755_v1.1         PVR 00083101
  745_v2.0         PVR 00083200
  755_v2.0         PVR 00083200
  755_v2.1         PVR 00083201
  745_v2.1         PVR 00083201
  755_v2.2         PVR 00083202
  745_v2.2         PVR 00083202
  755_v2.3         PVR 00083203
  745_v2.3         PVR 00083203
  755_v2.4         PVR 00083204
  745_v2.4         PVR 00083204
  755_v2.5         PVR 00083205
  745_v2.5         PVR 00083205
  755_v2.6         PVR 00083206
  745_v2.6         PVR 00083206
  755_v2.7         PVR 00083207
  745_v2.7         PVR 00083207
  755_v2.8         PVR 00083208
  755              (alias for 755_v2.8)
  goldfinger       (alias for 755_v2.8)
  745_v2.8         PVR 00083208
  745              (alias for 745_v2.8)
  750cxe_v2.4b     PVR 00083214
  750cxe_v3.1b     PVR 00083311
  750cxe           (alias for 750cxe_v3.1b)
  750cxr           PVR 00083410
  750cl_v1.0       PVR 00087200
  750cl_v2.0       PVR 00087210
  750cl            (alias for 750cl_v2.0)
  750l_v2.0        PVR 00088200
  750l_v2.1        PVR 00088201
  750l_v2.2        PVR 00088202
  750l_v3.0        PVR 00088300
  750l_v3.2        PVR 00088302
  750l             (alias for 750l_v3.2)
  lonestar         (alias for 750l_v3.2)
  604e_v1.0        PVR 00090100
  604e_v2.2        PVR 00090202
  604e_v2.4        PVR 00090204
  604e             (alias for 604e_v2.4)
  sirocco          (alias for 604e_v2.4)
  604r             PVR 000a0101
  mach5            (alias for 604r)
  7400_v1.0        PVR 000c0100
  7400_v1.1        PVR 000c0101
  7400_v2.0        PVR 000c0200
  7400_v2.1        PVR 000c0201
  7400_v2.2        PVR 000c0202
  7400_v2.6        PVR 000c0206
  7400_v2.7        PVR 000c0207
  7400_v2.8        PVR 000c0208
  7400_v2.9        PVR 000c0209
  7400             (alias for 7400_v2.9)
  g4               (alias for 7400_v2.9)
  970_v2.2         PVR 00390202
  970              (alias for 970_v2.2)
  970fx_v1.0       PVR 00391100
  power5p_v2.1     PVR 003b0201
  power5+          (alias for power5p_v2.1)
  power5+_v2.1     (alias for power5p_v2.1)
  power5gs         (alias for power5p_v2.1)
  970fx_v2.0       PVR 003c0200
  970fx_v2.1       PVR 003c0201
  970fx_v3.0       PVR 003c0300
  970fx_v3.1       PVR 003c0301
  970fx            (alias for 970fx_v3.1)
  ppc64            (alias for 970fx_v3.1)
  power7_v2.3      PVR 003f0203
  power7           (alias for power7_v2.3)
  970mp_v1.0       PVR 00440100
  970mp_v1.1       PVR 00440101
  970mp            (alias for 970mp_v1.1)
  power7p_v2.1     PVR 004a0201
  power7+          (alias for power7p_v2.1)
  power7+_v2.1     (alias for power7p_v2.1)
  power8e_v2.1     PVR 004b0201 (deprecated)
  power8e          (alias for power8e_v2.1)
  power8nvl_v1.0   PVR 004c0100 (deprecated)
  power8nvl        (alias for power8nvl_v1.0)
  power8_v2.0      PVR 004d0200
  power8           (alias for power8_v2.0)
  power9_v2.0      PVR 004e1200
  power9_v2.2      PVR 004e1202
  power9           (alias for power9_v2.2)
  power10_v2.0     PVR 00801200
  power10          (alias for power10_v2.0)
  g2               PVR 00810011
  mpc603           PVR 00810100
  g2hip3           PVR 00810101
  mpc8250_hip3     (alias for g2hip3)
  mpc8255_hip3     (alias for g2hip3)
  mpc8260_hip3     (alias for g2hip3)
  mpc8264_hip3     (alias for g2hip3)
  mpc8265_hip3     (alias for g2hip3)
  mpc8266_hip3     (alias for g2hip3)
  power11_v2.0     PVR 00821200
  power11          (alias for power11_v2.0)
  mpc8347t         PVR 00830010
  mpc8347          (alias for mpc8347t)
  mpc8347eap       PVR 00830010
  mpc8347p         PVR 00830010
  mpc8349          PVR 00830010
  e300c1           PVR 00830010
  mpc8343e         PVR 00830010
  mpc8347et        PVR 00830010
  mpc8347e         (alias for mpc8347et)
  mpc8343a         PVR 00830010
  mpc8349ea        PVR 00830010
  mpc8349e         PVR 00830010
  mpc8347at        PVR 00830010
  mpc8347a         (alias for mpc8347at)
  mpc8343ea        PVR 00830010
  mpc8347ep        PVR 00830010
  mpc8343          PVR 00830010
  mpc8349a         PVR 00830010
  mpc8347ap        PVR 00830010
  mpc8347eat       PVR 00830010
  mpc8347ea        (alias for mpc8347eat)
  e300c2           PVR 00840010
  e300c3           PVR 00850010
  e300             (alias for e300c3)
  mpc8379e         PVR 00860010
  e300c4           PVR 00860010
  mpc8377e         PVR 00860010
  mpc8378e         PVR 00860010
  mpc8379          PVR 00860010
  mpc8378          PVR 00860010
  mpc8377          PVR 00860010
  740p             PVR 10080000
  750p             PVR 10080000
  conan/doyle      (alias for 750p)
  460exb           PVR 130218a4
  460ex            (alias for 460exb)
  440epx           PVR 200008d0
  405d2            PVR 20010000
  x2vp4            PVR 20010820
  x2vp7            (alias for x2vp4)
  x2vp20           PVR 20010860
  x2vp50           (alias for x2vp20)
  405gpa           PVR 40110000
  405gpb           PVR 40110040
  405cra           PVR 40110041
  405gpc           PVR 40110082
  405gpd           PVR 401100c4
  405gp            (alias for 405gpd)
  405crb           PVR 401100c5
  405crc           PVR 40110145
  405cr            (alias for 405crc)
  405gpe           (alias for 405crc)
  stb03            PVR 40310000
  npe4gs3          PVR 40b10000
  npe405h          PVR 414100c0
  npe405h2         PVR 41410140
  405ez            PVR 41511460
  npe405l          PVR 416100c0
  stb04            PVR 41810000
  405d4            PVR 41810000
  405              (alias for 405d4)
  405lp            PVR 41f10000
  PPE42            PVR 42000000
  PPE42X           PVR 42100000
  PPE42XM          PVR 42200000
  440epa           PVR 42221850
  440epb           PVR 422218d3
  440ep            (alias for 440epb)
  405gpr           PVR 50910951
  405ep            PVR 51210950
  stb25            PVR 51510950
  750fx_v1.0       PVR 70000100
  750fx_v2.0       PVR 70000200
  750fx_v2.1       PVR 70000201
  750fx_v2.2       PVR 70000202
  750fl            PVR 70000203
  750fx_v2.3       PVR 70000203
  750fx            (alias for 750fx_v2.3)
  750gx_v1.0       PVR 70020100
  750gx_v1.1       PVR 70020101
  750gl            PVR 70020102
  750gx_v1.2       PVR 70020102
  750gx            (alias for 750gx_v1.2)
  440-xilinx-w-dfpu PVR 7ff21910
  440-xilinx       PVR 7ff21910
  7450_v1.0        PVR 80000100
  7450_v1.1        PVR 80000101
  7450_v1.2        PVR 80000102
  7450_v2.0        PVR 80000200
  7450_v2.1        PVR 80000201
  7450             (alias for 7450_v2.1)
  vger             (alias for 7450_v2.1)
  7441_v2.1        PVR 80000201
  7451_v2.3        PVR 80000203
  7451             (alias for 7451_v2.3)
  7441_v2.3        PVR 80000203
  7441             (alias for 7441_v2.3)
  7451_v2.10       PVR 80000210
  7441_v2.10       PVR 80000210
  7455_v1.0        PVR 80010100
  7445_v1.0        PVR 80010100
  7455_v2.1        PVR 80010201
  7445_v2.1        PVR 80010201
  7455_v3.2        PVR 80010302
  7455             (alias for 7455_v3.2)
  apollo6          (alias for 7455_v3.2)
  7445_v3.2        PVR 80010302
  7445             (alias for 7445_v3.2)
  7455_v3.3        PVR 80010303
  7445_v3.3        PVR 80010303
  7455_v3.4        PVR 80010304
  7445_v3.4        PVR 80010304
  7457_v1.0        PVR 80020100
  7447_v1.0        PVR 80020100
  7457_v1.1        PVR 80020101
  7447_v1.1        PVR 80020101
  7447             (alias for 7447_v1.1)
  7457_v1.2        PVR 80020102
  7457             (alias for 7457_v1.2)
  apollo7          (alias for 7457_v1.2)
  7457a_v1.0       PVR 80030100
  apollo7pm        (alias for 7457a_v1.0)
  7447a_v1.0       PVR 80030100
  7457a_v1.1       PVR 80030101
  7447a_v1.1       PVR 80030101
  7457a_v1.2       PVR 80030102
  7457a            (alias for 7457a_v1.2)
  7447a_v1.2       PVR 80030102
  7447a            (alias for 7447a_v1.2)
  mpc8641d         PVR 80040010
  mpc8641          PVR 80040010
  e600             PVR 80040010
  mpc8610          PVR 80040010
  7448_v1.0        PVR 80040100
  7448_v1.1        PVR 80040101
  7448_v2.0        PVR 80040200
  7448_v2.1        PVR 80040201
  7448             (alias for 7448_v2.1)
  7410_v1.0        PVR 800c1100
  7410_v1.1        PVR 800c1101
  7410_v1.2        PVR 800c1102
  7410_v1.3        PVR 800c1103
  7410_v1.4        PVR 800c1104
  7410             (alias for 7410_v1.4)
  nitro            (alias for 7410_v1.4)
  e500_v10         PVR 80200010
  mpc8540_v10      PVR 80200010
  mpc8560_v10      PVR 80200010
  mpc8541e_v11     PVR 80200020
  mpc8541e         (alias for mpc8541e_v11)
  mpc8555_v11      PVR 80200020
  mpc8555          (alias for mpc8555_v11)
  mpc8541e_v10     PVR 80200020
  mpc8555_v10      PVR 80200020
  e500_v20         PVR 80200020
  e500v1           (alias for e500_v20)
  mpc8560_v21      PVR 80200020
  mpc8560          (alias for mpc8560_v21)
  mpc8560_v20      PVR 80200020
  mpc8541_v11      PVR 80200020
  mpc8541          (alias for mpc8541_v11)
  mpc8555e_v11     PVR 80200020
  mpc8555e         (alias for mpc8555e_v11)
  mpc8541_v10      PVR 80200020
  mpc8555e_v10     PVR 80200020
  mpc8540_v21      PVR 80200020
  mpc8540          (alias for mpc8540_v21)
  mpc8540_v20      PVR 80200020
  mpc8548_v10      PVR 80210010
  mpc8543_v10      PVR 80210010
  e500v2_v10       PVR 80210010
  mpc8548e_v10     PVR 80210010
  mpc8543e_v10     PVR 80210010
  mpc8548_v11      PVR 80210011
  mpc8543_v11      PVR 80210011
  mpc8548e_v11     PVR 80210011
  mpc8543e_v11     PVR 80210011
  mpc8548e_v20     PVR 80210020
  mpc8545e_v20     PVR 80210020
  e500v2_v20       PVR 80210020
  mpc8548_v20      PVR 80210020
  mpc8543_v20      PVR 80210020
  mpc8545_v20      PVR 80210020
  mpc8547e_v20     PVR 80210020
  mpc8543e_v20     PVR 80210020
  mpc8545e_v21     PVR 80210021
  mpc8545e         (alias for mpc8545e_v21)
  mpc8533e_v10     PVR 80210021
  mpc8533_v10      PVR 80210021
  mpc8544_v10      PVR 80210021
  e500v2_v21       PVR 80210021
  mpc8548_v21      PVR 80210021
  mpc8548          (alias for mpc8548_v21)
  mpc8543_v21      PVR 80210021
  mpc8543          (alias for mpc8543_v21)
  mpc8545_v21      PVR 80210021
  mpc8545          (alias for mpc8545_v21)
  mpc8547e_v21     PVR 80210021
  mpc8547e         (alias for mpc8547e_v21)
  mpc8544e_v10     PVR 80210021
  mpc8543e_v21     PVR 80210021
  mpc8543e         (alias for mpc8543e_v21)
  mpc8548e_v21     PVR 80210021
  mpc8548e         (alias for mpc8548e_v21)
  mpc8568e         PVR 80210022
  mpc8533e_v11     PVR 80210022
  mpc8533e         (alias for mpc8533e_v11)
  mpc8533_v11      PVR 80210022
  mpc8533          (alias for mpc8533_v11)
  mpc8544_v11      PVR 80210022
  mpc8544          (alias for mpc8544_v11)
  e500v2_v22       PVR 80210022
  e500             (alias for e500v2_v22)
  e500v2           (alias for e500v2_v22)
  mpc8567e         PVR 80210022
  mpc8568          PVR 80210022
  mpc8567          PVR 80210022
  mpc8544e_v11     PVR 80210022
  mpc8544e         (alias for mpc8544e_v11)
  mpc8572          PVR 80210030
  mpc8572e         PVR 80210030
  e500v2_v30       PVR 80210030
  e500mc           PVR 80230020
  e5500            PVR 80240020
  e6500            PVR 80400020
  g2h4             PVR 80811010
  g2hip4           PVR 80811014
  mpc8241          (alias for g2hip4)
  mpc8245          (alias for g2hip4)
  mpc8250          (alias for g2hip4)
  mpc8250_hip4     (alias for g2hip4)
  mpc8255          (alias for g2hip4)
  mpc8255_hip4     (alias for g2hip4)
  mpc8260          (alias for g2hip4)
  mpc8260_hip4     (alias for g2hip4)
  mpc8264          (alias for g2hip4)
  mpc8264_hip4     (alias for g2hip4)
  mpc8265          (alias for g2hip4)
  mpc8265_hip4     (alias for g2hip4)
  mpc8266          (alias for g2hip4)
  mpc8266_hip4     (alias for g2hip4)
  g2le             PVR 80820010
  g2gp             PVR 80821010
  g2legp           PVR 80822010
  g2legp1          PVR 80822011
  mpc5200_v12      PVR 80822011
  mpc52xx          (alias for mpc5200_v12)
  mpc5200          (alias for mpc5200_v12)
  mpc5200b_v21     PVR 80822011
  mpc5200b         (alias for mpc5200b_v21)
  mpc5200_v11      PVR 80822011
  mpc5200b_v20     PVR 80822011
  mpc5200_v10      PVR 80822011
  g2legp3          PVR 80822013
  mpc82xx          (alias for g2legp3)
  powerquicc-ii    (alias for g2legp3)
  mpc8247          (alias for g2legp3)
  mpc8248          (alias for g2legp3)
  mpc8270          (alias for g2legp3)
  mpc8271          (alias for g2legp3)
  mpc8272          (alias for g2legp3)
  mpc8275          (alias for g2legp3)
  mpc8280          (alias for g2legp3)
  g2ls             PVR 90810010
  g2lels           PVR a0822010
```
Total items listed: 421

## 3. Supported Devices & Busses (`-device help`)

```text
Controller/Bridge/Hub devices:
name "i82378", bus PCI
name "i82801b11-bridge", bus PCI
name "ioh3420", bus PCI, desc "Intel IOH device id 3420 PCIE Root Port"
name "macio-newworld", bus PCI
name "macio-oldworld", bus PCI
name "pci-bridge", bus PCI, desc "Standard PCI Bridge"
name "pci-bridge-seat", bus PCI, desc "Standard PCI Bridge (multiseat)"
name "pcie-pci-bridge", bus PCI
name "pcie-root-port", bus PCI, desc "PCI Express Root Port"
name "pnv-phb", bus System
name "pnv-phb-root-port", bus PCI, desc "IBM PHB PCIE Root Port"
name "spapr-pci-host-bridge", bus System
name "usb-host", bus usb-bus
name "usb-hub", bus usb-bus
name "x3130-upstream", bus PCI, desc "TI X3130 Upstream Port of PCI Express Switch"
name "xio3130-downstream", bus PCI, desc "TI X3130 Downstream Port of PCI Express Switch"
USB devices:
name "ich9-usb-ehci1", bus PCI
name "ich9-usb-ehci2", bus PCI
name "ich9-usb-uhci1", bus PCI
name "ich9-usb-uhci2", bus PCI
name "ich9-usb-uhci3", bus PCI
name "ich9-usb-uhci4", bus PCI
name "ich9-usb-uhci5", bus PCI
name "ich9-usb-uhci6", bus PCI
name "nec-usb-xhci", bus PCI
name "pci-ohci", bus PCI, desc "Apple USB Controller"
name "piix3-usb-uhci", bus PCI
name "piix4-usb-uhci", bus PCI
name "qemu-xhci", bus PCI
name "usb-ehci", bus PCI
Storage devices:
name "160s33b", bus SSI, desc "Serial Flash"
name "25csm04", bus SSI, desc "Serial Flash"
name "320s33b", bus SSI, desc "Serial Flash"
name "640s33b", bus SSI, desc "Serial Flash"
name "am53c974", bus PCI, desc "AMD Am53c974 PCscsi-PCI SCSI adapter"
name "at25128a-nonjedec", bus SSI, desc "Serial Flash"
name "at25256a-nonjedec", bus SSI, desc "Serial Flash"
name "at25df041a", bus SSI, desc "Serial Flash"
name "at25df321a", bus SSI, desc "Serial Flash"
name "at25df641", bus SSI, desc "Serial Flash"
name "at25fs010", bus SSI, desc "Serial Flash"
name "at25fs040", bus SSI, desc "Serial Flash"
name "at26df081a", bus SSI, desc "Serial Flash"
name "at26df161a", bus SSI, desc "Serial Flash"
name "at26df321", bus SSI, desc "Serial Flash"
name "at26f004", bus SSI, desc "Serial Flash"
name "at45db081d", bus SSI, desc "Serial Flash"
name "dc390", bus PCI, desc "Tekram DC-390 SCSI adapter"
name "emmc", bus sd-bus, desc "eMMC"
name "en25f32", bus SSI, desc "Serial Flash"
name "en25p32", bus SSI, desc "Serial Flash"
name "en25p64", bus SSI, desc "Serial Flash"
name "en25q32b", bus SSI, desc "Serial Flash"
name "en25q64", bus SSI, desc "Serial Flash"
name "floppy", bus floppy-bus, desc "virtual floppy drive"
name "gd25q32", bus SSI, desc "Serial Flash"
name "gd25q64", bus SSI, desc "Serial Flash"
name "ich9-ahci", bus PCI, alias "ahci"
name "ide-cd", bus IDE, desc "virtual IDE CD-ROM"
name "ide-cf", bus IDE, desc "virtual CompactFlash card"
name "ide-hd", bus IDE, desc "virtual IDE disk"
name "is25lp016d", bus SSI, desc "Serial Flash"
name "is25lp032", bus SSI, desc "Serial Flash"
name "is25lp064", bus SSI, desc "Serial Flash"
name "is25lp080d", bus SSI, desc "Serial Flash"
name "is25lp128", bus SSI, desc "Serial Flash"
name "is25lp256", bus SSI, desc "Serial Flash"
name "is25lq040b", bus SSI, desc "Serial Flash"
name "is25wp032", bus SSI, desc "Serial Flash"
name "is25wp064", bus SSI, desc "Serial Flash"
name "is25wp128", bus SSI, desc "Serial Flash"
name "is25wp256", bus SSI, desc "Serial Flash"
name "isa-fdc", bus ISA, desc "virtual floppy controller"
name "isa-ide", bus ISA
name "lsi53c810", bus PCI
name "lsi53c895a", bus PCI, alias "lsi"
name "m25p05", bus SSI, desc "Serial Flash"
name "m25p10", bus SSI, desc "Serial Flash"
name "m25p128", bus SSI, desc "Serial Flash"
name "m25p16", bus SSI, desc "Serial Flash"
name "m25p20", bus SSI, desc "Serial Flash"
name "m25p32", bus SSI, desc "Serial Flash"
name "m25p40", bus SSI, desc "Serial Flash"
name "m25p64", bus SSI, desc "Serial Flash"
name "m25p80", bus SSI, desc "Serial Flash"
name "m25pe16", bus SSI, desc "Serial Flash"
name "m25pe20", bus SSI, desc "Serial Flash"
name "m25pe80", bus SSI, desc "Serial Flash"
name "m25px32", bus SSI, desc "Serial Flash"
name "m25px32-s0", bus SSI, desc "Serial Flash"
name "m25px32-s1", bus SSI, desc "Serial Flash"
name "m25px64", bus SSI, desc "Serial Flash"
name "m45pe10", bus SSI, desc "Serial Flash"
name "m45pe16", bus SSI, desc "Serial Flash"
name "m45pe80", bus SSI, desc "Serial Flash"
name "megasas", bus PCI, desc "LSI MegaRAID SAS 1078"
name "megasas-gen2", bus PCI, desc "LSI MegaRAID SAS 2108"
name "mptsas1068", bus PCI, desc "LSI SAS 1068"
name "mt25ql01g", bus SSI, desc "Serial Flash"
name "mt25ql02g", bus SSI, desc "Serial Flash"
name "mt25ql512ab", bus SSI, desc "Serial Flash"
name "mt25qu01g", bus SSI, desc "Serial Flash"
name "mt25qu02g", bus SSI, desc "Serial Flash"
name "mt35xu01g", bus SSI, desc "Serial Flash"
name "mt35xu02gbba", bus SSI, desc "Serial Flash"
name "mx25l12805d", bus SSI, desc "Serial Flash"
name "mx25l12855e", bus SSI, desc "Serial Flash"
name "mx25l1606e", bus SSI, desc "Serial Flash"
name "mx25l2005a", bus SSI, desc "Serial Flash"
name "mx25l25635e", bus SSI, desc "Serial Flash"
name "mx25l25635f", bus SSI, desc "Serial Flash"
name "mx25l25655e", bus SSI, desc "Serial Flash"
name "mx25l3205d", bus SSI, desc "Serial Flash"
name "mx25l4005a", bus SSI, desc "Serial Flash"
name "mx25l6405d", bus SSI, desc "Serial Flash"
name "mx25l8005", bus SSI, desc "Serial Flash"
name "mx66l1g45g", bus SSI, desc "Serial Flash"
name "mx66l51235f", bus SSI, desc "Serial Flash"
name "mx66u1g45g", bus SSI, desc "Serial Flash"
name "mx66u51235f", bus SSI, desc "Serial Flash"
name "n25q00", bus SSI, desc "Serial Flash"
name "n25q00a", bus SSI, desc "Serial Flash"
name "n25q032", bus SSI, desc "Serial Flash"
name "n25q032a11", bus SSI, desc "Serial Flash"
name "n25q032a13", bus SSI, desc "Serial Flash"
name "n25q064", bus SSI, desc "Serial Flash"
name "n25q064a11", bus SSI, desc "Serial Flash"
name "n25q064a13", bus SSI, desc "Serial Flash"
name "n25q128", bus SSI, desc "Serial Flash"
name "n25q128a11", bus SSI, desc "Serial Flash"
name "n25q128a13", bus SSI, desc "Serial Flash"
name "n25q256a", bus SSI, desc "Serial Flash"
name "n25q256a11", bus SSI, desc "Serial Flash"
name "n25q256a13", bus SSI, desc "Serial Flash"
name "n25q512a", bus SSI, desc "Serial Flash"
name "n25q512a11", bus SSI, desc "Serial Flash"
name "n25q512a13", bus SSI, desc "Serial Flash"
name "n25q512ax3", bus SSI, desc "Serial Flash"
name "nvdimm", desc "DIMM memory module"
name "nvme", bus PCI, desc "Non-Volatile Memory Express"
name "nvme-ns", bus nvme-bus, desc "Virtual NVMe namespace"
name "nvme-subsys", desc "Virtual NVMe subsystem"
name "pvscsi", bus PCI
name "s25fl016k", bus SSI, desc "Serial Flash"
name "s25fl064k", bus SSI, desc "Serial Flash"
name "s25fl129p0", bus SSI, desc "Serial Flash"
name "s25fl129p1", bus SSI, desc "Serial Flash"
name "s25fl256s0", bus SSI, desc "Serial Flash"
name "s25fl256s1", bus SSI, desc "Serial Flash"
name "s25fl512s", bus SSI, desc "Serial Flash"
name "s25fs512s", bus SSI, desc "Serial Flash"
name "s25sl004a", bus SSI, desc "Serial Flash"
name "s25sl008a", bus SSI, desc "Serial Flash"
name "s25sl016a", bus SSI, desc "Serial Flash"
name "s25sl032a", bus SSI, desc "Serial Flash"
name "s25sl032p", bus SSI, desc "Serial Flash"
name "s25sl064a", bus SSI, desc "Serial Flash"
name "s25sl064p", bus SSI, desc "Serial Flash"
name "s25sl12800", bus SSI, desc "Serial Flash"
name "s25sl12801", bus SSI, desc "Serial Flash"
name "s70fl01gs", bus SSI, desc "Serial Flash"
name "s70fs01gs", bus SSI, desc "Serial Flash"
name "scsi-cd", bus SCSI, desc "virtual SCSI CD-ROM"
name "scsi-hd", bus SCSI, desc "virtual SCSI disk"
name "sd-card", bus sd-bus
name "sd-card-spi", bus sd-bus, desc "SD SPI"
name "sdhci-pci", bus PCI
name "sii3112", bus PCI, desc "SiI3112A SATA controller"
name "spapr-nvdimm", desc "DIMM memory module"
name "spapr-vscsi", bus spapr-vio-bus
name "sst25vf016b", bus SSI, desc "Serial Flash"
name "sst25vf032b", bus SSI, desc "Serial Flash"
name "sst25vf040b", bus SSI, desc "Serial Flash"
name "sst25vf080b", bus SSI, desc "Serial Flash"
name "sst25wf010", bus SSI, desc "Serial Flash"
name "sst25wf020", bus SSI, desc "Serial Flash"
name "sst25wf040", bus SSI, desc "Serial Flash"
name "sst25wf080", bus SSI, desc "Serial Flash"
name "sst25wf512", bus SSI, desc "Serial Flash"
name "ufs", bus PCI, desc "Universal Flash Storage"
name "usb-bot", bus usb-bus
name "usb-storage", bus usb-bus
name "usb-uas", bus usb-bus
name "virtio-blk-device", bus virtio-bus
name "virtio-blk-pci", bus PCI, alias "virtio-blk"
name "virtio-blk-pci-non-transitional", bus PCI
name "virtio-blk-pci-transitional", bus PCI
name "virtio-scsi-device", bus virtio-bus
name "virtio-scsi-pci", bus PCI, alias "virtio-scsi"
name "virtio-scsi-pci-non-transitional", bus PCI
name "virtio-scsi-pci-transitional", bus PCI
name "w25q01jvq", bus SSI, desc "Serial Flash"
name "w25q02jvm", bus SSI, desc "Serial Flash"
name "w25q256", bus SSI, desc "Serial Flash"
name "w25q32", bus SSI, desc "Serial Flash"
name "w25q32dw", bus SSI, desc "Serial Flash"
name "w25q512jv", bus SSI, desc "Serial Flash"
name "w25q64", bus SSI, desc "Serial Flash"
name "w25q80", bus SSI, desc "Serial Flash"
name "w25q80bl", bus SSI, desc "Serial Flash"
name "w25x10", bus SSI, desc "Serial Flash"
name "w25x16", bus SSI, desc "Serial Flash"
name "w25x20", bus SSI, desc "Serial Flash"
name "w25x32", bus SSI, desc "Serial Flash"
name "w25x40", bus SSI, desc "Serial Flash"
name "w25x64", bus SSI, desc "Serial Flash"
name "w25x80", bus SSI, desc "Serial Flash"
Network devices:
name "e1000", bus PCI, alias "e1000-82540em", desc "Intel Gigabit Ethernet"
name "e1000-82544gc", bus PCI, desc "Intel Gigabit Ethernet"
name "e1000-82545em", bus PCI, desc "Intel Gigabit Ethernet"
name "e1000e", bus PCI, desc "Intel 82574L GbE Controller"
name "eTSEC", bus System, desc "Freescale Enhanced Three-Speed Ethernet Controller"
name "i82550", bus PCI, desc "Intel i82550 Ethernet"
name "i82551", bus PCI, desc "Intel i82551 Ethernet"
name "i82557a", bus PCI, desc "Intel i82557A Ethernet"
name "i82557b", bus PCI, desc "Intel i82557B Ethernet"
name "i82557c", bus PCI, desc "Intel i82557C Ethernet"
name "i82558a", bus PCI, desc "Intel i82558A Ethernet"
name "i82558b", bus PCI, desc "Intel i82558B Ethernet"
name "i82559a", bus PCI, desc "Intel i82559A Ethernet"
name "i82559b", bus PCI, desc "Intel i82559B Ethernet"
name "i82559c", bus PCI, desc "Intel i82559C Ethernet"
name "i82559er", bus PCI, desc "Intel i82559ER Ethernet"
name "i82562", bus PCI, desc "Intel i82562 Ethernet"
name "i82801", bus PCI, desc "Intel i82801 Ethernet"
name "igb", bus PCI, desc "Intel 82576 Gigabit Ethernet Controller"
name "ne2k_isa", bus ISA
name "ne2k_pci", bus PCI
name "pcnet", bus PCI
name "rocker", bus PCI, desc "Rocker Switch"
name "rtl8139", bus PCI
name "spapr-vlan", bus spapr-vio-bus
name "sungem", bus PCI
name "tulip", bus PCI
name "usb-net", bus usb-bus
name "virtio-net-device", bus virtio-bus
name "virtio-net-pci", bus PCI, alias "virtio-net"
name "virtio-net-pci-non-transitional", bus PCI
name "virtio-net-pci-transitional", bus PCI
name "vmxnet3", bus PCI, desc "VMWare Paravirtualized Ethernet v3"
Input devices:
name "adb-keyboard", bus apple-desktop-bus
name "adb-mouse", bus apple-desktop-bus
name "ccid-card-emulated", bus ccid-bus, desc "emulated smartcard"
name "ccid-card-passthru", bus ccid-bus, desc "passthrough smartcard"
name "i8042", bus ISA
name "ipoctal232", bus IndustryPack, desc "GE IP-Octal 232 8-channel RS-232 IndustryPack"
name "isa-parallel", bus ISA
name "isa-serial", bus ISA
name "pci-serial", bus PCI
name "pci-serial-2x", bus PCI
name "pci-serial-4x", bus PCI
name "spapr-vty", bus spapr-vio-bus
name "tpci200", bus PCI, desc "TEWS TPCI200 IndustryPack carrier"
name "usb-braille", bus usb-bus
name "usb-ccid", bus usb-bus, desc "CCID Rev 1.1 smartcard reader"
name "usb-kbd", bus usb-bus
name "usb-mouse", bus usb-bus
name "usb-serial", bus usb-bus
name "usb-tablet", bus usb-bus
name "usb-wacom-tablet", bus usb-bus, desc "QEMU PenPartner Tablet"
name "virtconsole", bus virtio-serial-bus
name "virtio-keyboard-device", bus virtio-bus
name "virtio-keyboard-pci", bus PCI, alias "virtio-keyboard"
name "virtio-mouse-device", bus virtio-bus
name "virtio-mouse-pci", bus PCI, alias "virtio-mouse"
name "virtio-multitouch-device", bus virtio-bus
name "virtio-multitouch-pci", bus PCI
name "virtio-serial-device", bus virtio-bus
name "virtio-serial-pci", bus PCI, alias "virtio-serial"
name "virtio-serial-pci-non-transitional", bus PCI
name "virtio-serial-pci-transitional", bus PCI
name "virtio-tablet-device", bus virtio-bus
name "virtio-tablet-pci", bus PCI, alias "virtio-tablet"
name "virtserialport", bus virtio-serial-bus
Display devices:
name "ati-vga", bus PCI
name "bochs-display", bus PCI
name "cirrus-vga", bus PCI, desc "Cirrus CLGD 54xx VGA"
name "secondary-vga", bus PCI
name "sm501", bus PCI, desc "SM501 Display Controller"
name "VGA", bus PCI
name "virtio-gpu-device", bus virtio-bus
name "virtio-gpu-pci", bus PCI, alias "virtio-gpu"
name "virtio-vga", bus PCI
Sound devices:
name "AC97", bus PCI, alias "ac97", desc "Intel 82801AA AC97 Audio"
name "adlib", bus ISA, desc "Yamaha YM3812 (OPL2)"
name "cs4231a", bus ISA, desc "Crystal Semiconductor CS4231A"
name "ES1370", bus PCI, alias "es1370", desc "ENSONIQ AudioPCI ES1370"
name "gus", bus ISA, desc "Gravis Ultrasound GF1"
name "hda-duplex", bus HDA, desc "HDA Audio Codec, duplex (line-out, line-in)"
name "hda-micro", bus HDA, desc "HDA Audio Codec, duplex (speaker, microphone)"
name "hda-output", bus HDA, desc "HDA Audio Codec, output-only (line-out)"
name "ich9-intel-hda", bus PCI, desc "Intel HD Audio Controller (ich9)"
name "intel-hda", bus PCI, desc "Intel HD Audio Controller (ich6)"
name "sb16", bus ISA, desc "Creative Sound Blaster 16"
name "usb-audio", bus usb-bus
name "virtio-sound-device", bus virtio-bus
name "virtio-sound-pci", bus PCI, alias "virtio-sound", desc "Virtio Sound"
Misc devices:
name "acpi-erst", bus PCI, desc "ACPI Error Record Serialization Table (ERST) device"
name "at24c-eeprom", bus i2c-bus
name "ctucan_pci", bus PCI, desc "CTU CAN PCI"
name "ds1338", bus i2c-bus
name "edu", bus PCI
name "guest-loader", desc "Guest Loader"
name "i2c-ddc", bus i2c-bus
name "i2c-echo", bus i2c-bus
name "i82374", bus ISA, desc "Intel 82374 DMA controller"
name "iommu-testdev", bus PCI, desc "A test device for IOMMU"
name "kvaser_pci", bus PCI, desc "Kvaser PCICANx"
name "loader", desc "Generic Loader"
name "m41t80", bus i2c-bus
name "mc146818rtc", bus ISA
name "mioe3680_pci", bus PCI, desc "Mioe3680 PCICANx"
name "pc-testdev", bus ISA
name "pca9535", bus i2c-bus
name "pca9552", bus i2c-bus
name "pca9554", bus i2c-bus
name "pci-testdev", bus PCI, desc "PCI Test Device"
name "pcm3680_pci", bus PCI, desc "Pcm3680i PCICANx"
name "pvpanic-pci", bus PCI
name "spapr-rng"
name "uefi-vars-sysbus", bus System
name "uefi-vars-x64", bus System
name "usb-redir", bus usb-bus
name "virtio-balloon-device", bus virtio-bus
name "virtio-balloon-pci", bus PCI, alias "virtio-balloon"
name "virtio-balloon-pci-non-transitional", bus PCI
name "virtio-balloon-pci-transitional", bus PCI
name "virtio-crypto-device", bus virtio-bus
name "virtio-crypto-pci", bus PCI
name "virtio-iommu-device", bus virtio-bus
name "virtio-iommu-pci", bus PCI, alias "virtio-iommu"
name "virtio-rng-device", bus virtio-bus
name "virtio-rng-pci", bus PCI, alias "virtio-rng"
name "virtio-rng-pci-non-transitional", bus PCI
name "virtio-rng-pci-transitional", bus PCI
CPU devices:
name "970_v2.2-spapr-cpu-core"
name "970mp_v1.0-spapr-cpu-core"
name "970mp_v1.1-spapr-cpu-core"
name "power10_v2.0-spapr-cpu-core"
name "power11_v2.0-spapr-cpu-core"
name "power5p_v2.1-spapr-cpu-core"
name "power7_v2.3-spapr-cpu-core"
name "power7p_v2.1-spapr-cpu-core"
name "power8_v2.0-spapr-cpu-core"
name "power8e_v2.1-spapr-cpu-core"
name "power8nvl_v1.0-spapr-cpu-core"
name "power9_v2.0-spapr-cpu-core"
name "power9_v2.2-spapr-cpu-core"
Watchdog devices:
name "i6300esb", bus PCI, desc "Intel 6300ESB"
name "ib700", bus ISA, desc "iBASE 700"
Uncategorized devices:
name "ipmi-bmc-extern"
name "ipmi-bmc-sim"
name "isa-ipmi-bt", bus ISA
name "isa-m48t59", bus ISA
name "pc-dimm", desc "DIMM memory module"
name "pnv-N1-chiplet", desc "PowerNV n1 chiplet"
name "pnv-nest-chiplet-pervasive", desc "PowerNV nest pervasive chiplet"
name "prep-systemio", bus ISA
name "rs6000-mc", bus ISA
name "spapr-tpm-proxy"
name "ufs-lu", bus ufs-bus, desc "Virtual UFS logical unit"
```
Total items listed: 372

## 4. Storage Image Formats (`-drive format=help`)

```text
Supported formats: blkdebug blklogwrites blkreplay blkverify bochs cloop compress copy-before-write copy-on-read dmg file ftp ftps host_device http https luks nbd null-aio null-co parallels preallocate qcow qcow2 qed quorum raw replication snapshot-access throttle vdi vhdx vmdk vpc vvfat
Supported formats (read-only): blkdebug blklogwrites blkreplay blkverify bochs cloop compress copy-before-write copy-on-read dmg file ftp ftps host_device http https luks nbd null-aio null-co parallels preallocate qcow qcow2 qed quorum raw replication snapshot-access throttle vdi vhdx vmdk vpc vvfat
```
Total items listed: 2

## 5. Network Backends (`-netdev help`)

```text
Available netdev backend types:
socket
stream
dgram
hubport
tap
passt
user
```
Total items listed: 8

## 6. Character Device Backends (`-chardev help`)

```text
Available chardev backend types:
  stdio
  spicevmc
  vc
  serial
  pipe
  console
  testdev
  null
  hub
  ringbuf
  wctablet
  qemu-vdagent
  msmouse
  dbus
  socket
  udp
  file
  spiceport
  mux
  memory
```
Total items listed: 21

## 7. Display Backends (`-display help`)

```text
Available display backend types:
none
curses
spice-app
dbus
Some display backends support suboptions, which can be set with
   -display backend,option=value,option=value...
For a short list of the suboptions for each display, see the top-level -help output; more detail is in the documentation.
```
Total items listed: 8

## 8. Audio Drivers (`-audiodev help`)

```text
Available audio drivers:
none
dbus
dsound
spice
wav
```
Total items listed: 6

## 9. Accelerators (`-accel help`)

```text
Accelerators supported in QEMU binary:
tcg
```
Total items listed: 2

## 10. User Objects (`-object help`)

```text
List of user creatable objects:
  authz-list
  authz-list-file
  authz-simple
  can-bus
  colo-compare
  cryptodev-backend
  cryptodev-backend-builtin
  dbus-display
  dbus-vmstate
  filter-buffer
  filter-dump
  filter-mirror
  filter-redirector
  filter-replay
  filter-rewriter
  input-barrier
  iothread
  main-loop
  memory-backend-ram
  pef-guest
  qtest
  rng-builtin
  rng-egd
  secret
  throttle-group
  tls-cipher-suites
  tls-creds-anon
  tls-creds-psk
  tls-creds-x509
```
Total items listed: 30

## AQEMU Wizard Rules for this Architecture

1. **Machine Whitelist:** Only select machines that match strings in Section 1.
2. **Bus Attachment Matching:** Devices in Section 3 specify their required bus (e.g. `bus PCI`, `bus ISA`, `bus virtio-bus`). Do not place PCI devices on ISA-only machines like `isapc`.
3. **Network Backend Validation:** Use Section 5 backends (e.g. `user`, `tap`, `socket`).
4. **Storage Drivers:** Only attach block drivers matching Section 4 formats.
