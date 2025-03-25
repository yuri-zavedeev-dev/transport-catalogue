# transport-catalogue

## Описание

Это учебный проект, выполненный в рамках [курса повышения квалификации от Яндекс Практикума](https://practicum.yandex.ru/cpp/?from=catalog). Он разрабатывался на протяжении почти всего периода обучения, постепенно получая новый функционал. На текущий момент транспортный справочник умеет:
* Читать из стандартного потока ввода JSON-файл, содержащий настройки справочника и некоторое количество запросов на получение информации
* Выводить в стандартный поток вывода JSON-файл с ответами на ранее полученные запросы
* Обрабатывать запрос на получение информации об остановке или маршруте
* Обрабатывать запрос на построение кратчайшего маршрута между двумя остановками (в том числе с пересадками)
* Обрабатывать запрос на построение карты маршрутов в виде svg-изображения

Проект разрабатывался длительное время, поэтапно, поэтому содержит как удачные решения, так и не очень. Однако на его примере были изучены различные возможности языка и его особенности.
## Изученные технологии

* STL (в проекте использовано большое количество контейнеров и алгоритмов стандартной библиотеки)
* Работа со строками и `std::string_view`
* Работа с итераторами
* Работа со ссылочными типами
* Паттерны проектирования(строитель, фабричный метод, шаблонный метод, посетитель)
* Шаблонные методы и шаблонные классы
## Файловая структура

Для удобства работы с проектом я сформировал следующую файловую структуру:
* docs/
* include/
	* json/
	* map/
	* router/
	* transport_catalogue/
	* util/
* src/
	* json/
	* map/
	* router/
	* transport_catalogue/
	* util/

В директории docs/ хранится документация проекта.  В include/ лежат заголовочные файлы, а в src/ - файлы исходного кода. В папках json/ лежат файлы относящиеся к парсеру JSON-формата, конструктор для объектов JSON и вспомогательные классы. В map/ лежат файлы для работы с svg изображениями и отрисовки курты маршрутов. В router/ лежат файлы транспортного маршрутизатора, а также вспомогательные файлы от Яндекса для построения взвешенного графа (`graph.hpp/cpp`, `router.hpp/cpp`). transport_catalogue/ хранит файлы самого транспортного справочника, обработчика запросов и файлы `domain.hpp/cpp`, в которых хранятся повсеместно используемые структуры. Также в src/transport_catalogue/ лежит `main.cpp`, в котором определена точка входа в приложение. Директория util/ хранит вспомогательные классы для работы справочника.
## Сборка

В корне каталога лежат CMakeLists.txt, и CMakePresets.json настроенный для сборки Debug версии проекта. Можно создать папку build/ в корне каталога и выполнить в ней команды CMake:

* `cmake ../ -DCMAKE_BUILD_TYPE=Debug -G "MinGW Makefiles"`
* `cmake --build .`

Или расширением для работы с CMake для vscode.

## Документация

На текущий момент документация к проекту - диаграмма классов на языке PlantUML. Её можно просмотреть через инструмент - [такой](https://plantuml-editor.kkeisuke.com/) или [такой](https://www.plantuml.com/plantuml/uml/SyfFKj2rKt3CoKnELR1Io4ZDoSa700003).
Сейчас диаграмма достаточна большая, так как вмещает в себя почти все классы проекта, поэтому в ближайшее время я планирую разбить её на несколько отдельных самостоятельных диаграмм.
