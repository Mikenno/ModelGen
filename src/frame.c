
#include <string.h>

#include "frame.h"
#include "debug.h"


void mgCreateStackFrameEx(MGStackFrame *frame, MGValue *module, MGValue *locals)
{
	MG_ASSERT(frame);
	MG_ASSERT(module);
	MG_ASSERT(module->type == MG_TYPE_MODULE);
	MG_ASSERT(locals);

	memset(frame, 0, sizeof(MGStackFrame));

	frame->state = MG_STACK_FRAME_STATE_ACTIVE;
	frame->module = module;
	frame->locals = locals;
}


void mgDestroyStackFrame(MGStackFrame *frame)
{
	MG_ASSERT(frame);

	if (frame->value)
		mgDestroyValue(frame->value);

	mgDestroyValue(frame->module);
	mgDestroyValue(frame->locals);
}
