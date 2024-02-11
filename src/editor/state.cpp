#include "state.hpp"

void EditorState::UpdateState(EditorState& source) {
  // update this
  gridManager.UpdateState(source.gridManager);
  flush = source.flush;

  // reset source
  source.flush = false;
}
