//
// Created by ab-flies on 15/11/24.
//

#ifndef GUI_H
#define GUI_H

#include "PipeHandler.h"

using PipeWriter = PipeHandler::PipeWriter;
using PipeReader = PipeHandler::PipeReader;

void ncursesGui(PipeWriter&& pipeWriter, PipeReader&& pipeReader);
void initWindows();
void printMenu();
void printLogs();
void printTableView();
void listenPipe(PipeReader& pipeReader);
void handleUserInput(int ch);

#endif //GUI_H
