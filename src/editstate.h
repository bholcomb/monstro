#ifndef MONSTRO_INTERNAL_EDITSTATE_H
#define MONSTRO_INTERNAL_EDITSTATE_H

#include <cstddef>
#include <cstdint>

namespace monstro
{

struct EditState
{
   uint64_t myId = 0;
   char *myBuf = nullptr;
   size_t myBufSize = 0;
   int myCursorPos = 0;
   int mySelectStart = 0;
   int mySelectEnd = 0;
   bool myHasSelection = false;
   float myCursorBlinkTimer = 0.0f;

   void clear();
   bool isActive() const { return myId != 0; }
   bool isActiveFor(uint64_t id) const { return myId == id; }

   void activate(uint64_t id, char *buf, size_t buf_size, bool select_all);
   void deactivate();

   void deleteSelection();
   void insertText(const char *text);
   void selectAll();

   int getSelectionStart() const;
   int getSelectionEnd() const;
};

} // namespace monstro

#endif
