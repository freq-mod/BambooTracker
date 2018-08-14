#ifndef SET_KEY_RELEASE_TO_STEP_QT_COMMAND_HPP
#define SET_KEY_RELEASE_TO_STEP_QT_COMMAND_HPP

#include <QUndoCommand>
#include "gui/pattern_editor/pattern_editor_panel.hpp"

class SetKeyReleaseToStepQtCommand : public QUndoCommand
{
public:
	SetKeyReleaseToStepQtCommand(PatternEditorPanel* panel, QUndoCommand* parent = nullptr);
	void redo() Q_DECL_OVERRIDE;
	void undo() Q_DECL_OVERRIDE;
	int id() const Q_DECL_OVERRIDE;

private:
	PatternEditorPanel* panel_;
};

#endif // SET_KEY_RELEASE_TO_STEP_QT_COMMAND_HPP
