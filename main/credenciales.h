#ifndef __cred__.h
#define __cred__.h

/* WIFI CREDENCIALES */
// EDGE 
#define EDGE_AP             "ESP-AP"
#define EDGE_PASS           "123456789"
//#define EDGE_AP             "HUAWEI-2.4G-NmNg"
//#define EDGE_PASS           "nd55W3kD"


// MODEM
#define MODEM_AP            "WIFILOCAL"
#define MODEM_PASS          "123123123"


/* EDGE BOX ENDPOINTS */

#define edge_server         "10.42.0.1:5000"
//#define edge_server         "192.168.18.196:5000"

#define edge_salud_data     "/salud/datos"
#define edge_salud_size     "/salud/size"
#define edge_pesaje_data    "/pesaje/datos"
#define edge_pesaje_size    "/pesaje/size"
#define edge_localtime      "/datetime"


/* TPI ENDPOINTS */
#define tpi_server          "https://omnicloud.sitech.com.pe/api/"
#define tpi_pesaje          "OperacionesExternal"
#define tpi_salud           "OperacionesExternal/CargarAutomaticaSaludEquipos"
#define tpi_key             "41FFC272-4960-484F-998F-CB981EE3E01B"
#define tpi_format          "application/json"

/* CST ENDPOINTS */
#define cst_server          "http://20.206.129.111:1880/"
#define cst_salud           "salud"
#define cst_bateria         "dispositivo/salud"
#define cst_pesaje          "pesaje"

#endif