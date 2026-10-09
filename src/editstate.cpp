#include "editstate.h"
#include <algorithm>
#include <cstring>

namespace monstro
{

static int clamp_index(int value, int min_value, int max_value)
{
   if (value < min_value)
      return min_value;
   if (value > max_value)
      return max_value;
   return value;
}

void EditState::clear()
{
   myId = 0;
   myCursorPos = 0;
   mySelectStart = 0;
   mySelectEnd = 0;
   myHasSelection = false;
   myCursorBlinkTimer = 0.0f;
}

void EditState::activate(uint64_t id, const char *buf, bool select_all)
{
   myId = id;
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

void EditState::deleteSelection(char *buf)
{
   if (!myHasSelection)
      return;

   int len = (int)strlen(buf);
   myCursorPos = clamp_index(myCursorPos, 0, len);
   mySelectStart = clamp_index(mySelectStart, 0, len);
   mySelectEnd = clamp_index(mySelectEnd, 0, len);

   int sel_start = getSelectionStart();
   int sel_end = getSelectionEnd();

   if (sel_start >= sel_end)
      return;

   memmove(buf + sel_start, buf + sel_end, len - sel_end + 1);
   myCursorPos = sel_start;
   myHasSelection = false;
   mySelectStart = sel_start;
   mySelectEnd = sel_start;
}

void EditState::insertText(char *buf, size_t buf_size, const char *text)
{
   if (!text)
      return;

   if (myHasSelection)
   {
      deleteSelection(buf);
   }

   int len = (int)strlen(buf);
   myCursorPos = clamp_index(myCursorPos, 0, len);
   mySelectStart = clamp_index(mySelectStart, 0, len);
   mySelectEnd = clamp_index(mySelectEnd, 0, len);

   int insert_len = (int)strlen(text);
   int available = (int)buf_size - len - 1;
   if (available <= 0)
      return;
   if (insert_len > available)
      insert_len = available;

   memmove(buf + myCursorPos + insert_len, buf + myCursorPos, len - myCursorPos + 1);
   memcpy(buf + myCursorPos, text, insert_len);
   myCursorPos += insert_len;
   mySelectStart = myCursorPos;
   mySelectEnd = myCursorPos;
}

void EditState::selectAll(const char *buf)
{
   int len = (int)strlen(buf);
   mySelectStart = 0;
   mySelectEnd = len;
   myCursorPos = len;
   myHasSelection = len > 0;
}

} // namespace monstro
