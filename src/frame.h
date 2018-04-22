#ifndef MODELGEN_FRAME_H
#define MODELGEN_FRAME_H

#define _MG_STACK_FRAME_STATES \
	_MG_SFS(ACTIVE, "Active") \
	_MG_SFS(RETURN, "Return") \
	_MG_SFS(BREAK, "Break") \
	_MG_SFS(CONTINUE, "Continue")


#define _MG_LONGEST_STACK_FRAME_STATE_NAME_LENGTH 9


typedef enum MGStackFrameState {
#define _MG_SFS(state, name) MG_STACK_FRAME_STATE_##state,
	_MG_STACK_FRAME_STATES
#undef _MG_SFS
} MGStackFrameState;


static char *_MG_STACK_FRAME_STATE_NAMES[] = {
#define _MG_SFS(state, name) name,
	_MG_STACK_FRAME_STATES
#undef _MG_SFS
};

#endif