#include <SDL.h>
#include <stdio.h>

#include "dynawave.h"

void DynaWave::wavetable_write(uint16_t address, uint8_t value) {
	state.wavetable[address & 0xFFF] = value;
}

uint8_t DynaWave::wavetable_read(uint16_t address) {
	return state.wavetable[address & 0xFFF];
}

uint8_t volume_convert[8] = {252, 141, 80, 45, 25, 14, 8, 0};

void DynaWave::register_write(uint16_t address, uint8_t value) {
	state.regs[address & 7] = value;
	//periods for SQUARE1 and SQUARE2 represent number of 44.1k output samples between toggles
	state.periods[SQUARE1] = (state.regs[SQUARE1_NOTE] << ((state.regs[SQUARE1_CTRL] & 7) + 5)) / 325;
	state.volumes[SQUARE1] = volume_convert[(state.regs[SQUARE1_CTRL] >> 3) & 0x7];
	state.periods[SQUARE2] = (state.regs[SQUARE2_NOTE] << ((state.regs[SQUARE2_CTRL] & 7) + 5)) / 325;
	state.volumes[SQUARE2] = volume_convert[(state.regs[SQUARE2_CTRL] >> 3) & 0x7];

	state.periods[NOISE] = state.regs[NOISE_CTRL] & 7;
	state.volumes[NOISE] = volume_convert[(state.regs[NOISE_CTRL] >> 3) & 0x7];
}

void fill_audio(void *udata, uint8_t *stream, int len) {
	AudioState *state = (AudioState*) udata;
	for(int i = 0; i < len; i++) {
		state->clocks[SQUARE1] --;
		if(state->clocks[SQUARE1] == 0) {
			state->clocks[SQUARE1] = state->periods[SQUARE1];
			state->out[SQUARE1] = !(state->out[SQUARE1]) * state->volumes[SQUARE1];
		}
		state->clocks[SQUARE2] --;
		if(state->clocks[SQUARE2] == 0) {
			state->clocks[SQUARE2] = state->periods[SQUARE2];
			state->out[SQUARE2] = !(state->out[SQUARE2]) * state->volumes[SQUARE2];
		}

		for(int n = 0; n <= state->periods[NOISE]; n++) {
			state->clocks[NOISE] = (state->clocks[NOISE] << 1) | (!!(state->clocks[NOISE] & 0x8000) ^ !!(state->clocks[NOISE] & 0x0100));
		}
		state->out[NOISE] = !!(state->clocks[NOISE] & 0x8000) * state->volumes[NOISE];

		stream[i] = (state->out[SQUARE2] + state->out[SQUARE1] + state->out[NOISE]) / 4;
	}
	//SDL_MixAudio(stream, mix_buffer, len, SDL_MIX_MAXVOLUME / 2);
}

DynaWave::DynaWave() {
	SDL_AudioSpec wanted;

	state.clocks[0] = 1;
	state.clocks[1] = 1;
	state.clocks[2] = 1;
	state.clocks[3] = 1;

	state.periods[0] = 1;
	state.periods[1] = 1;
	state.periods[2] = 1;
	state.periods[3] = 1;

	state.volumes[0] = 0;
	state.volumes[1] = 0;
	state.volumes[2] = 0;
	state.volumes[3] = 0;

	state.out[0] = 0;
	state.out[1] = 0;

    /* Set the audio format */
    wanted.freq = 44100;
    wanted.format = AUDIO_U8;
    wanted.channels = 1;    /* 1 = mono, 2 = stereo */
    wanted.samples = 1024;  /* Good low-latency value for callback */
    wanted.callback = fill_audio;
    wanted.userdata = &state;

    /* Open the audio device, forcing the desired format */
    if ( SDL_OpenAudio(&wanted, NULL) < 0 ) {
        fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
    } else {
    	SDL_PauseAudio(0);
    }


	return;
}