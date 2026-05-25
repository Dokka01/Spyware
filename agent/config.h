#ifndef CONFIG_H
#define CONFIG_H

#define SERVER_BASE_URL  "http://127.0.0.1:5000"
#define REPORT_URL       SERVER_BASE_URL "/api/machines/report"
#define BUFFER_SIZE      256
#define DATA_BUFFER      4096
#define LARGE_BUFFER     65536   /* pour browser history et apps */
#define SCREENSHOT_FILE  "screenshot.bmp"

#endif /* CONFIG_H */
