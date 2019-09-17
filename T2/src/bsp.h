/*
 * bsp.h
 *
 *  Created on: 29 Şub 2016
 *      Author: admin
 */
/*Üretim İçin Hex :
 *
  Üretim için 1. imaj 0x10000 adresinden itibaren 0x30000 (192KB).
  Flash'ın aşağıdaki şekilde ayarlanması lazım.
  Applicationda'da IAP ip/port yazmak için kullanıldığından ötürü RAM'in son 64 byte'ı kullanılmıyor ve
  RAM size 0x7fc0 olarak giriliyor.

  Bootloader HEX ve application HEX notepad'de uç uca eklenerek üretim dosyası oluşturuluıyor.
  Üretim dosyası oluşturuken bootloaderın son satırındaki :00000001FF'in silinmesi gerekiyor.
  Bootloader'ın öne kopyalanması gerekiyor.

2) Update için Bin :
   İmajın memorynin 2. bölümüne yazılıp kullanılabilmesi için flash'ın 0x40000 adresine yazılması lazım.
   Boyut yine 0x30000 olacak.
*/
#ifndef BSP_H_
#define BSP_H_

#define DEVICE_MODEL       "T2"
#define SW_VERSION        "R0071"
#define BUILD_DATE        __DATE__

#define GPS_PORT                 LPC_UART2
#define VERSION                  (DEVICE_MODEL"-"SW_VERSION";"BUILD_DATE";")

#define NUM_DIG_OUTPUTS             2
#define NUM_DIGITAL_INPUTS          6
#define SYSTICK_MS                  10
#endif /* BSP_H_ */
