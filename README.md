# Házi feladat

# Feladat kiírás
A minta feladat egy *IoT alkalmazás* elkészítése. A beágyazott Linux rendszeren futó grafikus felhasználói felület nélküli alkalmazás egy I2C buszon kommunikáló szenzor segítségével adatokat (nyomás, hőmérséklet, páratartalom) gyűjt a környezetről. A beágyazott platformon futó alkalmazáshoz klienssel TCP protokollon keresztül kapcsolódhatunk. A kliens oldalon parancssorból küldött utasításokat a beágyazott platformon futó alkalmazás értelmezi. Ilyen módon lehetőség van a szenzor konfigurálására, az adatok lekérésére és törlésére. A konfigurációk és kapcsolódások ténye naplózásra/logolásra kerül.

A program az alábbi funkciókkal fog rendelkezni:
* A beágyazott platformon az alkalmazás daemonként fut.
* TCP protokollon kapcsolódó kliensek azonosítása (felhasználónév, jelszó, csoport).
* Több kliens párhuzamos kiszolgálásának támogatása (Thread Pool).
* Szerveresemények naplózása (syslog).
* Klienscsoporttól függően a mért adatokhoz és a szenzorbeállításokhoz való hozzáférés ellenőrzése (írás, olvasás jogok).
* Megosztott változók és állományok esetén kölcsönös kizárás biztosítása.
* Környezeti adatok ciklikus mérése és fájlba mentése, továbbá azok lekérdezésének és törlésének lehetősége.

# Megvalósított program
Az elkészült program az előző pontban kitűzött valamennyi funkciót teljesíti. Az eredeti kiíráshoz képest különbség, hogy a felhasználó által küldött parancsok értelmezése már kliens oldalon megtörténik, melyet bájtokban kódolva kap meg a szerver, így csökkenthető a hálózaton továbbítandó adatmennyiség és a szerver leterheltsége. A futtatható állományok előállítására szolgáló paraméterezett parancs (gcc) kliens esetén a mysensor.c, szerver esetén a myserver.c álományban található és fordítás helyétől függően a -I csatolót kell átírni. A szerver esetén lehetőségünk van "debug barát" fordításra a -DSERVER_DEBUG csatoló használatával. Ekkor a szerver nem fog daemonként futni és a logok a standard kimeneten is megjelennek. Szerver indítása a ./myserver parancs futtatásával lehetséges. Kliens indítása a ./mysensor parancs futtatásával lehetséges, opcionálisan IP cím és PORT szám megadásával.
