#pragma once

void ErrorHandling(int errCode, bool isExit = true);
void ErrorHandling(const char* msg, bool isExit = true);
void ErrorHandling(const char* msg, int errCode, bool isExit = true);
