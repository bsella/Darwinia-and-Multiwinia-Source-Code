#ifndef INCLUDED_MAIN_H
#define INCLUDED_MAIN_H

extern double   g_gameTime;					  // Updated from GetHighResTime every frame
extern double   g_startTime;
extern float    g_advanceTime;                // How long the last frame took
extern double   g_lastServerAdvance;          // Time of last server advance
extern float    g_predictionTime;             // Time between last server advance and start of render
extern float    g_targetFrameRate;
extern int      g_lastProcessedSequenceId;

void AppMain();

#endif
