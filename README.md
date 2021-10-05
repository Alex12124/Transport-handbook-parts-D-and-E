# Транспортный справочник Часть D

Гарантируется, что вещественные числа не задаются в экспоненциальной записи, то есть обязательно имеют целую часть и, возможно, дробную часть через десятичную точку.

Каждый запрос к базе дополнительно получает идентификатор в поле **id** — целое число от 0 до 2147483647. Ответ на запрос должен содержать идентификатор этого запроса в поле **request_id**. Это позволяет выводить ответы на запросы в любом порядке.

Ключи словарей могут располагаться в произвольном порядке. Форматирование (то есть пробельные символы вокруг скобок, запятых и двоеточий) не имеет значения как во входном, так и в выходном JSON.

Два файла json.cpp и json.h предоставляются

### Пример
**Представленные выше файлы решают часть E, которая содержит в себе часть D. Коммит для части D своевременно не был сохранен. Версия из части E некорректно отрабатывает запросы из части D, так как появляются настройки вроде routing settings в части E**

**Ввод**
```
{
  "base_requests": [
    {
      "type": "Stop",
      "road_distances": {
        "Marushkino": 3900
      },
      "longitude": 37.20829,
      "name": "Tolstopaltsevo",
      "latitude": 55.611087
    },
    {
      "type": "Stop",
      "road_distances": {
        "Rasskazovka": 9900
      },
      "longitude": 37.209755,
      "name": "Marushkino",
      "latitude": 55.595884
    },
    {
      "type": "Bus",
      "name": "750",
      "stops": [
        "Tolstopaltsevo",
        "Marushkino",
        "Rasskazovka"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Stop",
      "road_distances": {},
      "longitude": 37.333324,
      "name": "Rasskazovka",
      "latitude": 55.632761
    }
  ],
  "stat_requests": [
    {
      "type": "Bus",
      "name": "750",
      "id": 519139350
    },
    {
      "type": "Bus",
      "name": "751",
      "id": 194217464
    },
    {
      "type": "Stop",
      "name": "Samara",
      "id": 746888088
    },
    {
      "type": "Stop",
      "name": "Tolstopaltsevo",
      "id": 1042838872
    }
  ]
}
```
**Вывод**
```
[
  {
    "route_length": 27600,
    "request_id": 519139350,
    "curvature": 1.31808,
    "stop_count": 5,
    "unique_stop_count": 3
  },
  {
    "request_id": 194217464,
    "error_message": "not found"
  },
  {
    "request_id": 746888088,
    "error_message": "not found"
  },
  {
    "buses": [
      "750"
    ],
    "request_id": 1042838872
  }
]
```

# Транспортный справочник Часть E

### Изменения формата ввода

**Новая секция — routing_settings**

Во входной JSON добавляется ключ "routing_settings", значением которого является словарь с двумя ключами:

  *  "bus_wait_time" — время ожидания автобуса на остановке (в минутах). Считайте, что когда бы человек ни пришёл на остановку и какой бы ни была эта остановка, он будет ждать любой автобус в точности указанное количество минут. Значение — целое число от 1 до 1000.

  *  "bus_velocity" — скорость автобуса (в км/ч). Считайте, что скорость любого автобуса постоянна и в точности равна указанному числу. Время стоянки на остановках не учитывается, время разгона и торможения — тоже. Значение — вещественное число от 1 до 1000.
  
### Пример
```
"routing_settings": {
  "bus_wait_time": 6,
  "bus_velocity": 40
}
```
Данная конфигурация задаёт время ожидания равным 6 минутам и скорость автобусов равной 40 километрам в час.

**Новый тип запросов к базе — Route**

В список stat_requests добавляются элементы с "type": "Route" — это запросы на построение маршрута между двумя остановками. Помимо стандартных свойств "id" и "type", они содержат ещё два:

  * "from" — остановка, в которой нужно начать маршрут.

  * "to" — остановка, в которой нужно закончить маршрут.

Оба значения — названия существующих в базе остановок (однако, возможно, не принадлежащих ни одному автобусному маршруту).

### Пример

```
{
  "type": "Route",
  "from": "Biryulyovo Zapadnoye",
  "to": "Universam",
  "id": 4
}
```
Данный запрос означает построение маршрута от остановки «Biryulyovo Zapadnoye» до остановки «Universam».

На маршруте человек может использовать несколько автобусов, и даже один автобус несколько раз — если на некоторых участках он делает большой крюк и проще срезать на другом автобусе.

Маршрут должен быть наиболее оптимален по времени. Если маршрутов с минимально возможным суммарным временем несколько, допускается вывести любой из них: тестирующая система проверяет лишь совпадение времени маршрута с оптимальным и корректность самого маршрута.

При прохождении маршрута время расходуется на два типа активностей:

  * Ожидание автобуса. Всегда длится bus_wait_time минут.

  * Поездка на автобусе. Всегда длится ровно такое количество времени, которое требуется для преодоления данного расстояния со скоростью bus_velocity. Расстояние между остановками вычисляется по дорогам, то есть с помощью road_distances.

Ходить пешком, выпрыгивать из автобуса между остановками и использовать другие виды транспорта запрещается. На конечных остановках все автобусы высаживают пассажиров и уезжают в парк. Даже если человек едет на кольцевом ("is_roundtrip": true) маршруте и хочет проехать мимо конечной, он будет вынужден выйти и подождать тот же самый автобус ровно bus_wait_time минут. Этот и другие случаи разобраны в примерах.

Ответ на запрос Route устроен следующим образом:

```
{
    "request_id": <id запроса>,
    "total_time": <суммарное время>,
    "items": [
        <элементы маршрута>
    ]
}
```
total_time — суммарное время в минутах, требуемое для прохождения маршрута, выведенное в виде вещественного числа.

Обратите внимание, что расстояние от остановки A до остановки B может быть не равно расстоянию от B до A!

items — список элементов маршрута, каждый из которых описывает непрерывную активность пассажира, требующую временных затрат. А именно, элементы маршрута бывают двух типов.

**Wait** — подождать нужное количество минут (в нашем случае — всегда bus_wait_time) на указанной остановке:
```
{
    "type": "Wait",
    "stop_name": "Biryulyovo",
    "time": 6
}
```
**Bus** — проехать span_count остановок (перегонов между остановками) на автобусе bus, потратив указанное количество минут:  
```
{
    "type": "Bus",
    "bus": "297",
    "span_count": 2,
    "time": 5.235
}
```
В случае отсутствия маршрута между указанными остановками выведите результат в следующем формате:
```
{
    "request_id": <id запроса>,
    "error_message": "not found"
}
```
Предоставлены две небольшие библиотеки:
  * graph.h — класс, реализующий взвешенный ориентированный граф.

  * router.h — класс, реализующий поиск кратчайшего пути во взвешенном ориентированном графе.
  
Вам необходимо:
  * придумать, как по транспортному справочнику построить граф, путь наименьшего веса в котором соответствует оптимальному маршруту на автобусах, предварительно определившись, что в этом графе будет являться вершинами, а что — рёбрами;

  * написать код построения графа и описания маршрута по пути, полученному от маршрутизатора.

Примеры ввода и вывда будут представлены в json файлах. Также будут представлены pdf файлы, которые показывают наглядно графы из входных файлов и построение маршрутов по ним.