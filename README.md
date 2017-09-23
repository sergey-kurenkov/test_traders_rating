[![Build Status](https://travis-ci.org/skwllsp/test_traders_rating.svg?branch=master)](https://travis-ci.org/skwllsp/test_traders_rating)

Написать микросервис на С++ для выдачи рейтинга трейдеров. От ядра системы в микросервис приходят следующие сообщения - user_registered(id,name), user_renamed(id,name), user_deal_won(id,time,amount), user_connected(id), user_disconnected(id).

Способ доставки(через сеть, файл, pipe, и т.д.) сообщений находится вне рамок текущего задания, выбор остается за Вами.

Сервис раз в минуту (и при user_connected) должен отсылать сообщение с содержимым рейтинга для конкретного пользователя: первые 10 позиций рейтинга, позицию юзера в рейтинге, +- 10 соседей по рейтингу для текущего пользователя. Рейтинг сортируется по обороту выигрышных сделок за календарную неделю( с 00:00 понедельника текущей недели).

Требования - сервис должен обеспечивать максимальную производительность для функционирования в high-load режиме.

Реализация сохранения состояния при рестарте микросервиса не входит в задание.
