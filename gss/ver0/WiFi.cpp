#include <string>
#include <core>

#defive VERSION "0.00"

// ---- Параметры RS485 (#define) ----
#define RS485_PORT          2     // Номер порта (RS485)
#define RS485_BAUD          19200 // Скорость обмена
#define RS485_TX_BUFSIZE    2048  // Размер глобального буфера отправки
#define RS485_RX_BUFSIZE    128   // Размер глобального буфера приема

// Дополнительные параметры (по умолчанию)
#define RS485_STOP_BITS     0     
#define RS485_PARITY        0     
#define RS485_PACKET_MODE   1     // 1 = Включен пакетный режим (обязательно для PortReadPackage)

// ---- Глобальные переменные ----
new tx_buf[RS485_TX_BUFSIZE]  // Буфер для отправки (если нужен)
new rx_buf[RS485_RX_BUFSIZE]  // Буфер для приема (ограничен 128 байтами)
// ---- Статусы (Булевые переменные: 0 = false, 1 = true) ----
new status_wifi_adapter = 0  // Статус ответа от вайфай адаптера
new status_wifi_config = 1  // Статус настройки вайфай модуля
new status_wifi_conn = 0  // Статус подключения вайфай к сети
new status_server_conn = 0  // Статус подключения к серверу
new status_server_auth = 0  // Статус авторизации на сервере

// ---- Строковые переменные ----
new IMEI{16} // Для  хранения IMEI

// Команды для управления модулем
new AT{} = "AT+\r\n"
new HOST{} = "AT+HOST=hub.wialonlocal.com\r\n"
new PORT{} = "AT+PORT=20332\r\n"
new SSID{} = "AT+SSID=TREKBERRY.RU-2.4\r\n"
new PASS{} = "AT+PASS=ПАРОЛЬ ВВЕДИ\r\n"
new WFCONN{} = "AT+WFCONN\r\n"
new WFDIS{} = "AT+WFDIS\r\n"
new RESET{} = "RESET\r\n"
new SERVCONN{} = "AT+SERVCONN\r\n"
new SERVDIS{} = "AT+SERVDIS\r\n"
new SEND{} = "AT+SEND="

LOGIN(){
    clear_tx_buf()
    strcat(tx_buf, SEND)
    strcat(tx_buf, "#L#")
    strcat(tx_buf, IMEI)
    strcat(tx_buf, ";NA\r\n")
    Diagnostics("LOGIN:%s", tx_buf)
    
    SendStringRS485(tx_buf)
    new timer = GetVar(UNIX_TIME)
    while(GetVar(UNIX_TIME) - timer < 2){
        if(ReadRS485() == true){
            Diagnostics(rx_buf)
            if(strfind(rx_buf, "+SEND:OK") == 0){
                clear_rx_buf()
                break
            }
        }
    }

    timer = GetVar(UNIX_TIME)
    while(GetVar(UNIX_TIME) - timer < 10){
        if(ReadRS485() == true){
            Diagnostics(rx_buf)
            if(strfind(rx_buf, "#AL#1") == 0){
                status_server_auth = 1
                clear_rx_buf()
                clear_tx_buf()
                break
            }
        }
    }
}

clear_tx_buf(){
    for (new i = 0; i < RS485_TX_BUFSIZE; i++) {
        tx_buf[i] = 0x00
    }
}

clear_rx_buf(){
    for (new i = 0; i < RS485_RX_BUFSIZE; i++) {
        rx_buf[i] = 0x00
    }
}
// ---- Функция получения IMEI терминала ----
GetIMEIFromDevice()
{
    new res = GetIMEI(IMEI, 16)    
    if (res == 1) {
        Diagnostics("IMEI записан: %s", IMEI)
        return true
    } else {
        Diagnostics("Ошибка: IMEI не считан")
        return false
    }
}

// ---- Функция инициализации ----
InitRS485()
{
    new res = PortInit(RS485_PORT, RS485_BAUD, RS485_TX_BUFSIZE, RS485_STOP_BITS, RS485_PARITY, RS485_PACKET_MODE)
    if (res == 1) {
        Diagnostics("RS485 Init OK")
        return 1
    }
    return 0
}

// ---- Функция отправки строки ----
SendStringRS485(const msg[])
{
    new len = strlen(msg)
    if (len > 0) {
        new written = PortWrite(RS485_PORT, msg, len)
        if (written == len) {
            return 1
        }
    }
    return 0
}

// ---- Функция чтения ----
ReadRS485()
{
    new i = 0
    new c
    while(PortRead(RS485_PORT, c, 50)){
        rx_buf{i} = c
        i++
    }
    if(i > 0){
        rx_buf{i+1} = 0x00
        return true
    }
    else{
        return false
    }
}

SERVConnect(){
    SendStringRS485(SERVCONN)
    new timer = GetVar(UNIX_TIME)
    while(GetVar(UNIX_TIME) - timer < 2){
        if(ReadRS485() == true){
            Diagnostics(rx_buf)
            if(strfind(rx_buf, "+SERVCONN:OK") == 0){
                status_server_conn = 1
                clear_rx_buf()
                break
            }
            else{
                status_server_conn = 0
                clear_rx_buf()
                break
            }
        }
    }
    if(status_server_conn == 1){
        Diagnostics("Подключение к серверу установлено")
    }
    else{
        Diagnostics("Ошибка: подключение к серверу не установлено")
    }
}

WIFIConnect(){
    SendStringRS485(WFCONN)
    new timer = GetVar(UNIX_TIME)
    while(GetVar(UNIX_TIME) - timer < 2){
        if(ReadRS485() == true){
            Diagnostics(rx_buf)
            if(strfind(rx_buf, "OK") == 0){
                status_wifi_conn = 1
                clear_rx_buf()
                break
            }
            else{
                status_wifi_conn = 0
                clear_rx_buf()
                break
            }
        }
    }
    new statusOK = 0
    if(status_wifi_conn == 1){
        timer = GetVar(UNIX_TIME)
        while(GetVar(UNIX_TIME) - timer < 30){
            if(ReadRS485() == true){
                Diagnostics(rx_buf)
                if(strfind(rx_buf, "+WFCONN:OK") == 0){
                    statusOK = 1
                    status_wifi_conn = 1
                    clear_rx_buf()
                    break
                }
            }
        }
    }
    if(statusOK == 0){
        status_wifi_conn = 0
        Diagnostics("Ошибка времени ожидания подключения к WiFi")
    }
    if(status_wifi_conn == 1){
        Diagnostics("Подключение к WiFi установлено")
    }
    else{
        Diagnostics("Ошибка: подключение к WiFi не установлено")
    }
}

wifiCFG(){
    SendStringRS485(HOST)
    new timer = GetVar(UNIX_TIME)
    while(GetVar(UNIX_TIME) - timer < 2){   
        if(ReadRS485() == true){
            if(strfind(rx_buf, "+HOST:") == 0){
                Diagnostics(rx_buf)
                status_wifi_config = 1
                clear_rx_buf()
                break
            }
            else{
                status_wifi_config = 0
                clear_rx_buf()
                break
            }
        }
    }
    if(status_wifi_config == 1){
        SendStringRS485(PORT)
        timer = GetVar(UNIX_TIME)
        while(GetVar(UNIX_TIME) - timer < 2){
            if(ReadRS485() == true){
                if(strfind(rx_buf, "+PORT:") == 0){
                    Diagnostics(rx_buf)
                    status_wifi_config = 1
                    clear_rx_buf()
                    break
                }
                else{
                    status_wifi_config = 0
                    clear_rx_buf()
                    break
                }
            }
        }
    }
    if(status_wifi_config == 1){
        SendStringRS485(SSID)
        timer = GetVar(UNIX_TIME)
        while(GetVar(UNIX_TIME) - timer < 2){
            if(ReadRS485() == true){
                if(strfind(rx_buf, "+SSID:") == 0){
                    Diagnostics(rx_buf)
                    status_wifi_config = 1
                    clear_rx_buf()
                    break
                }
                else{
                    status_wifi_config = 0
                    clear_rx_buf()
                    break
                }
            }
        }
    }
    if(status_wifi_config == 1){
        SendStringRS485(PASS)
        timer = GetVar(UNIX_TIME)
        while(GetVar(UNIX_TIME) - timer < 2){
            if(ReadRS485() == true){
                if(strfind(rx_buf, "+PASS:") == 0){
                    Diagnostics(rx_buf)
                    status_wifi_config = 1
                    clear_rx_buf()
                    break
                }
                else{
                    status_wifi_config = 0
                    clear_rx_buf()
                    break
                }
            }
        }
    }
    if(status_wifi_config == 1){
        Diagnostics("WiFi настроен")
    }
    else{
        Diagnostics("Ошибка: WiFi не настроен")
    }
}
// ---- Main ----
main()
{
    // Вызываем функцию
    GetIMEIFromDevice()    
    if (InitRS485() == 0) {
        // Если инициализация не удалась, выходим или повторяем
        Delay(5000)
        return
    }
    else{
    SendStringRS485(RESET)
    Delay(1000)
    while(true){
        if(status_wifi_adapter == 1 ){//Проверяем  есть ли ответ от WIFI модуля
            //Diagnostics("Есть ответ от WiFi модуля")
            if(status_wifi_config == 0){
                wifiCFG()
            }
            else{
                if(status_wifi_conn == 0){
                    WIFIConnect()
                }
                if(status_wifi_conn == 1 & status_server_conn == 0){
                    SERVConnect()
                }        
                if(status_server_conn == 1 & status_server_auth == 0){
                    LOGIN()
                }
                if(status_server_auth == 1){
                    while(true){
                        Diagnostics("Отправка данных")
                        SendStringRS485("AT+SEND=#M#TEST\r\n")
                        Delay(1000)
                    }
                }
            }            
        }
        else{//Если ответа нет, то пытаемя получить ответ
            SendStringRS485(AT) //Посылаем запрос
            new timer = GetVar(UNIX_TIME)
            while(GetVar(UNIX_TIME) - timer < 2){ //Тут  мы 2 секунды ждем ответа
                if(ReadRS485() == true){
                    if(strfind(rx_buf, "OK") == 0){
                        status_wifi_adapter = 1
                        clear_rx_buf()
                        break
                    }
                }
            }
        }
    }
    }
}
