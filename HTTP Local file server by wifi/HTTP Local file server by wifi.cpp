#include <stdio.h>  
#include <WinSock2.h>
#include <WS2tcpip.h>

//HTTP thì không cần lưu lại mảng socket, vì trao đổi xong là nó sẽ đóng, không giữ kết nối nữa
//13p. giao thuc POST

void concat(char** result, const char* folder, const char* filename) {
	int oldLen = (*result == NULL ? 0 : strlen(*result));
	int contentLen = strlen(filename);
	int tagLen = 19 + strlen(filename) + strlen(folder);
	*result = (char*)realloc(*result, oldLen + contentLen + tagLen + 1);
	memset(*result + oldLen, 0, contentLen + tagLen + 1);
	sprintf(*result + oldLen, "<a href=\"%s%s\">%s</a><br>", folder, filename, filename);
}

void concat(char** result, char* tmp) {
	int oldLen = (*result == NULL ? 0 : strlen(*result));
	*result = (char*)realloc(*result, oldLen + strlen(tmp) + 1);
	memset(*result + oldLen, 0, strlen(tmp) + 1);
	sprintf(*result + oldLen, "%s", tmp);
}

int isFoler(char* path) {
	if (strcmp(path, "/") == 0) return 1; // / la thu muc root
	WIN32_FIND_DATAA finddata;
	char findpath[1024];
	memset(findpath, 0, sizeof(findpath));
	sprintf(findpath, "C:%s", path); // Khong co *.*, sau do xuong duoi lai bo ca dau xuoc cuoi cung --> tim chinh findpath đó
	if (findpath[strlen(findpath) - 1] == '/')
		findpath[strlen(findpath) - 1] = 0; //bo dau xuoc cuoi cung
	HANDLE hFind = FindFirstFileA(findpath, &finddata);
	if (hFind != INVALID_HANDLE_VALUE) {
		if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { //Không có ngoặc sai vỡ mồm, vì & thực hiện sau !=
			//is folder 
			return 1;
		}
		else {
			//is not folder
			return 0;
		}
	}
	else {
		return -1;
	}

}

char* scanFolder(const char* folder) {   //     "C:\..."
	char* result = NULL;
	concat(&result, (char*)"<html>");
	concat(&result, (char*)"<form action=\"/\" method=\"post\" enctype=\"multipart/form-data\"><input type=\"file\" id=\"file\" name=\"file\"><br><br><input type=\"submit\" name=\"submit\" value=\"Submit\" ></form>");

	char findpath[1024]; //    "C:\*.*"
	memset(findpath, 0, sizeof(findpath));
	sprintf(findpath, "C:%s*.*", folder); // Có *.* --> Tìm các file/folder con của findpath đó
	//Include "A" : ASCII version, not include: unicode version
	WIN32_FIND_DATAA finddata;
	HANDLE hFind = FindFirstFileA(findpath, &finddata);
	if (hFind != INVALID_HANDLE_VALUE) {
		concat(&result, folder, finddata.cFileName);
		while (FindNextFileA(hFind, &finddata)) {
			concat(&result, folder, finddata.cFileName);
		}
	};
	concat(&result, (char*)"</html>");
	//printf("%s", result);
	return result;
}

void chuanhoaTenFolder(char* path) {
	if (strcmp(path, "/") == 0) return;
	if (path[strlen(path) - 1] != '/') {
		path[strlen(path)] = '/';
	}
	//thay %20 bang dau cach:
	char* search = NULL;
	while (true) {
		search = strstr(path, "%20");
		if (search == NULL) {
			break;
		}
		else {
			search[0] = 32; //32 is space
			strcpy(search + 1, search + 3);
		}
	}
}

DWORD WINAPI ClientThread(LPVOID param) {
	SOCKET c = (SOCKET)param;
	char buf[1024];
	memset(buf, 0, sizeof(buf));
	recv(c, buf, sizeof(buf), 0);
	printf("%s\n", buf); //Header nhan duoc

	//GET / HTTP/1.1  --> GET, or POST is action, the rest is called path
	char action[1024];  memset(action, 0, sizeof(action));
	char path[1024];    memset(path, 0, sizeof(path));
	sscanf(buf, "%s %s", action, path);  printf("%s   %s\n", action, path);
	if (strcmp(action, "GET") == 0) {
		chuanhoaTenFolder(path);
		if (isFoler(path) == 1) {
			char* html = scanFolder(path);
			char* response = (char*)calloc(1024 + strlen(html), 1);
			sprintf(response, "HTTP/1.1 200 OK\r\nServer:HOANGDUONGMINHDUY\r\nContent_Type: text/html\r\nContent-Length: %d\r\n\r\n%s",
				strlen(html), html);
			send(c, response, strlen(response), 0);
			closesocket(c);
			free(response); response = NULL;
			free(html);  html = NULL;
		}
		else if (isFoler(path) == 0) {
			if (path[strlen(path) - 1] == '/') path[strlen(path) - 1] = 0;


			char type[1024]; memset(type, 0, 1024);
			if (strcmp(path + strlen(path) - 4, ".mp4") == 0) {
				strcpy(type, "video/mp4"); //http mp4 content type
			}
			else if (strcmp(path + strlen(path) - 4, ".mp3") == 0) {
				strcpy(type, "audio/mp3");
			}
			else if (strcmp(path + strlen(path) - 4, ".jpg") == 0) {
				strcpy(type, "image/jpg");
			}
			else if (strcmp(path + strlen(path) - 4, ".txt") == 0) {
				strcpy(type, "text/plain");
			}
			else if (strcmp(path + strlen(path) - 4, ".pdf") == 0) {
				strcpy(type, "application/pdf");
			}
			else if (strcmp(path + strlen(path) - 4, ".cpp") == 0) {
				strcpy(type, "text/cpp");
			}
			else {
				strcpy(type, "application/octet-stream"); //luồng byte
			}

			char fullpath[1024];
			memset(fullpath, 0, sizeof(fullpath));
			sprintf(fullpath, "C:\\%s", path);
			FILE* f = fopen(fullpath, "rb"); //do k biet truoc kieu file nen phai read binary
			fseek(f, 0, SEEK_END); // nhảy 0 byte từ vị trí cuối file, tức là đưa con trỏ về cuối file
			int flen = ftell(f);   // lấy vị trí của con trỏ trong file f (hiện tại đang ở cuối file)
			fseek(f, 0, SEEK_SET); // đưa con trỏ về đầu file để đọc
			char* data = (char*)calloc(flen, 1);
			if (data != NULL) {
				fread(data, 1, flen, f); //đọc toàn bộ file f vào data
				char response[1024];
				memset(response, 0, sizeof(response));
				sprintf(response, "HTTP/1.1 200 OK\r\nServer:HOANGDUONGMINHDUY\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n", type, flen);
				send(c, response, strlen(response), 0); //Gửi header OK HTTP trước 
				send(c, data, flen, 0); //Rồi mới gửi dữ liệu
				free(data);
				closesocket(c);
			}
			fclose(f);
		}
		else {
			//404 error
			char response[1024];
			memset(response, 0, 1024);
			char* html = (char*)"<html>404 E RÒ, PHAI NÓT PHAO</html>";
			sprintf(response, "HTTP/1.1 404 Not Found\r\nServer:HOANGDUONGMINHDUY\r\Content-Length: %d\r\n\r\n%s", (int)strlen(html), html);
			send(c, response, strlen(response), 0);
			closesocket(c);
		}

	}

	if (strcmp(action, "POST") == 0) {
		char response[1024];
		memset(response, 0, 1024);
		sprintf(response, "HTTP/1.1 200 OK\r\nServer:HOANGDUONGMINHDUY\r\n\r\n");
		send(c, response, strlen(response), 0);

		char tmp[1024];
		memset(tmp, 0, 1024);
		recv(c, tmp, 1024, 0);
		printf("%s\n\n", tmp);
	}
	return 0;
}

void main() {
	printf("HELLO cmcm\n");
	WSADATA wdata;
	WSAStartup(MAKEWORD(2, 2), &wdata);
	SOCKADDR_IN saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(8888);
	saddr.sin_addr.S_un.S_addr = 0;

	//HTTP hoat dong dua tren nen TCP =)))
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	bind(s, (SOCKADDR*)& saddr, sizeof(saddr));
	listen(s, 10);
	while (0 == 0) {
		SOCKADDR_IN caddr;
		int clen = sizeof(caddr);
		SOCKET c = accept(s, (SOCKADDR*)& caddr, &clen);
		CreateThread(NULL, 0, ClientThread, (LPVOID)c, 0, NULL);
	}
}