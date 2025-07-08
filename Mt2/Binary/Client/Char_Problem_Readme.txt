falls noch andere sprachen fixed werden müssen muss man das eben auch erstmal manuell machen... das kostet sonst so zu viel zeit denke ich. logs sind per ENABLE_CHAR_INPUT_LOG deaktiviert, 
sind zu aktivieren falls man neue nicht schreibbare chars adden will in die liste
zeigt dann in syserr beim schreiben alle nötigen indexe an und dann beim c&p zeigt er auch alle indexes an (zum replacen) 
und mit den angezeigten daten kann mans dann einfach in die liste in IME.cpp einfügen (const wchar_t wcReplaceTable[][3] = {)
sind comments dran was was ist