#ifndef LOGGER_H_INCLUDED
#define LOGGER_H_INCLUDED



///registra los eventos del sistema en el archivo sistema.log

void logEvent(int jobId, int prioridad, const char* evento);

#endif // LOGGER_H_INCLUDED
