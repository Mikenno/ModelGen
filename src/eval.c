
#include "eval.h"
#include "module.h"


extern MGValue* _mgVisitNode(MGValue *module, MGNode *node);


MGValue* mgEvalEx(MGInstance *instance, const char *string, const MGValue *locals)
{
	MG_ASSERT(instance);
	MG_ASSERT(string);
	MG_ASSERT(!locals || (locals->type == MG_VALUE_MAP));

	MGValue *module = mgCreateValueModule();
	module->data.module.instance = instance;

	MGNode *root = mgParseString(&module->data.module.parser, string);
	MG_ASSERT(root);
	MG_ASSERT(_mgListLength(root->children) == 1);

	MGStackFrame frame;
	mgCreateStackFrame(&frame, mgReferenceValue(module));

	if (locals)
		mgMapMerge(frame.locals, locals, MG_TRUE);

	mgPushStackFrame(instance, &frame);

	MGValue *value = _mgVisitNode(module, _mgListGet(root->children, 0));
	MG_ASSERT(value);

	if ((value->type == MG_VALUE_FUNCTION) && locals && mgMapSize(locals))
	{
		if (value->data.func.locals)
			mgMapMerge(value->data.func.locals, locals, MG_TRUE);
		else
			value->data.func.locals = mgReferenceValue(module->data.module.instance->callStackTop->locals);
	}

	mgPopStackFrame(instance, &frame);
	mgDestroyStackFrame(&frame);

	mgDestroyValue(module);

	return value;
}
