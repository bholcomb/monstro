#include "editstate.h"
#include <algorithm>
#include <cstring>

namespace monstro
{

void EditState::clear()
{
   myId = 0;
   myBuf = nullptr;
   myBufSize = 0;
   myCursorPos = 0;
   mySelectStart = 0;
   mySelectEnd = 0;
   myHasSelection = false;
   myCursorBlinkTimer = 0.0f;
}

void EditState::activate(uint64_t id, char *buf, size_t buf_size, bool select_all)
{
   myId = id;
   myBuf = buf;
   myBufSize = buf_size;
   myCursorBlinkTimer = 0.0f;

   int len = (int)strlen(buf);
   if (select_all)
   {
      myCursorPos = len;
      mySelectStart = 0;
      mySelectEnd = len;
      myHasSelection = len > 0;
   }
   else
   {
      myCursorPos = len;
      mySelectStart = len;
      mySelectEnd = len;
      myHasSelection = false;
   }
}

void EditState::deactivate() { clear(); }

int EditState::getSelectionStart() const { return std::min(mySelectStart, mySelectEnd); }

int EditState::getSelectionEnd() const { return std::max(mySelectStart, mySelectEnd); }

void EditState::deleteSelection()
{
   if (!myHasSelection || !myBuf)
      return;

   int sel_start = getSelectionStart();
   int sel_end = getSelectionEnd();
   int len = (int)strlen(myBuf);

   if (sel_start >= sel_end)
      return;

   memmove(myBuf + sel_start, myBuf + sel_end, len - sel_end + 1);
   myCursorPos = sel_start;
   myHasSelection = false;
   mySelectStart = sel_start;
   mySelectEnd = sel_start;
}

void EditState::insertText(const char *text)
{
   if (!myBuf || !text)
      return;

   if (myHasSelection)
   {
      deleteSelection();
   }

   int len = (int)strlen(myBuf);
   int insert_len = (int)strlen(text);
   int available = (int)myBufSize - len - 1;
   if (available <= 0)
      return;
   if (insert_len > available)
      insert_len = available;

   memmove(myBuf + myCursorPos + insert_len, myBuf + myCursorPos, len - myCursorPos + 1);
   memcpy(myBuf + myCursorPos, text, insert_len);
   myCursorPos += insert_len;
   mySelectStart = myCursorPos;
   mySelectEnd = myCursorPos;
}

void EditState::selectAll()
{
   if (!myBuf)
      return;
   int len = (int)strlen(myBuf);
   mySelectStart = 0;
   mySelectEnd = len;
   myCursorPos = len;
   myHasSelection = len > 0;
}

} // namespace monstro
