#ifndef MONSTRO_INTERNAL_EDITSTATE_H
#define MONSTRO_INTERNAL_EDITSTATE_H

#include <cstddef>
#include <cstdint>

namespace monstro
{

// The context's one active text edit. It keeps the cursor and selection
// between frames but never the caller's buffer: every operation takes the
// buffer passed to the widget this frame, so a caller may reallocate or move
// it freely.
struct EditState
{
   uint64_t myId = 0;
   int myCursorPos = 0;
   int mySelectStart = 0;
   int mySelectEnd = 0;
   bool myHasSelection = false;
   float myCursorBlinkTimer = 0.0f;

   // Text of the numeric field being edited; numeric widgets have no caller text buffer.
   char myNumberText[64] = {};

   void clear();
   bool isActive() const { return myId != 0; }
   bool isActiveFor(uint64_t id) const { return myId == id; }

   void activate(uint64_t id, const char *buf, bool select_all);
   void deactivate();

   void deleteSelection(char *buf);
   void insertText(char *buf, size_t buf_size, const char *text);
   void selectAll(const char *buf);

   int getSelectionStart() const;
   int getSelectionEnd() const;
};

} // namespace monstro

#endif
