// Own test document: preprocessor directives are trivia, always skipped --
// the `#else` branch's own text is discarded bodily, whatever it contains.

#if WITH_EDITOR
#endif

#ifdef DEBUG_BUILD
void DebugOnly()
{
}
#else
this is not valid angelscript at all, and that is the point: it is skipped
#endif

void Real()
{
	int32 x = 0;
}
