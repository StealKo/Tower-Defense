Pro zkompilování programu bude muset uživatel použít následující příkaz z hlavní složky.

mkdir build
cd build
cmake ..
make

Zároveň musí mít nainstalované knihovny SDL, SDL_image, SDL_ttf a tyhle složky musí mít v hlavním souboru, aby šel program zkompilovat.

Kdyby to i tak nefungovalo, tak lze stáhnout tyhle soubory zde - https://github.com/StealKo/Tower-Defense

Následně musí spustit hru z build složky.

./main


Vysvětlení programu:

Celý program se ovládá pouze myší, kdy uživatel může stavět jednotlivé věže na vyznačená místečka, která mají jinačí texturu (hnědá textura).

Jakmile uživatel klikne na vyznačené políčko, tak se mu uprostřed mapy objeví menu, kde si může jednotlivé věže postavit a nebo vylepšit, pokud už nějaká věž na tom místu je postavená.

Uživatel si může vybrat mezi 3 typy věží:
- Default tower -> normální věž, která střílí a dává poškození
- Slow Tower -> zpomaluje monstra na určitou dobu
- Fast Tower -> normální věž, která má větší rychlost střelby, než Default Tower

Jakmile uživatel spustí program, tak má 5 sekund než se spawne první vlna monster a mezi jednotlivými vlnami je též 5 sekundová pauza.

Za každé zabité monstrum dostane uživatel peníze a skóre, které se navyšuje od třetí vlny, protože monstra jsou od třetí vlny silnější a odolnější.

Pokud se nějaké monstrum dostane na konec, tak uživateli ubere životy podle poškození monstra a pokud uživatel už žádné životy nebude mít tak se hra vypne a vypíše se do konzole "Uživatel byl poražen".