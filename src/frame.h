#ifndef MODELGEN_STACK_FRAME_H
#define MODELGEN_STACK_FRAME_H

#include "value.h"
#include "types/composite.h"

#define _MG_STACK_FRAME_STATES \
	_MG_SFS(ACTIVE, "Active") \
	_MG_SFS(RETURN, "Return") \
	_MG_SFS(BREAK, "Break") \
	_MG_SFS(CONTINUE, "Continue")

#define _MG_LONGEST_STACK_FRAME_STATE_NAME_LENGTH 9

static char *_MG_STACK_FRAME_STATE_NAMES[] = {
#define _MG_SFS(state, name) name,
		_MG_STACK_FRAME_STATES
#undef _MG_SFS
};

typedef enum MGStackFrameState {
#define _MG_SFS(state, name) MG_STACK_FRAME_STATE_##state,
	_MG_STACK_FRAME_STATES
#undef _MG_SFS
} MGStackFrameState;

typedef struct MGStackFrame MGStackFrame;

typedef struct MGStackFrame {
	MGStackFrameState state;
	MGStackFrame *last;
	MGStackFrame *next;
	MGValue *module;
	const MGNode *caller;
	const char *callerName;
	MGValue *value;
	MGValue *locals;
} MGStackFrame;

#define mgCreateStackFrame(frame, module) mgCreateStackFrameEx(frame, module, mgCreateValueMap(1 << 4))
void mgCreateStackFrameEx(MGStackFrame *frame, MGValue *module, MGValue *locals);
void mgDestroyStackFrame(MGStackFrame *frame);

#endif