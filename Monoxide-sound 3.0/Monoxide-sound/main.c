#include "sound.h"

#pragma region Public Variables
INT nCounter = 0;
#pragma endregion Public Variables

VOID WINAPI Initialize(VOID)
{
	SeedXorshift32((DWORD)__rdtsc());
	InitializeSine();
}

INT WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ PWSTR pszCmdLine,
	_In_ INT nShowCmd
)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(pszCmdLine);
	UNREFERENCED_PARAMETER(nShowCmd);

	HANDLE hAudioThread;

	Initialize();

	Sleep(5000);

	Sleep(1000);

	CreateMutexW(NULL, TRUE, L"Monoxide.exe");

	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		exit;
		return 0;
	}

	pAudioSequences[0] = (AUDIO_SEQUENCE_PARAMS){ 48000, 48000 * 30, AudioSequence1,  NULL, NULL };
	pAudioSequences[1] = (AUDIO_SEQUENCE_PARAMS){ 8000,  8000 * 30, AudioSequence2,  NULL, NULL };
	pAudioSequences[2] = (AUDIO_SEQUENCE_PARAMS){ 8000,  8000 * 30, AudioSequence3,  NULL, NULL };
	pAudioSequences[3] = (AUDIO_SEQUENCE_PARAMS){ 16000, 16000 * 30, AudioSequence4,  NULL, NULL };
	pAudioSequences[4] = (AUDIO_SEQUENCE_PARAMS){ 8000,  8000 * 30, AudioSequence5,  NULL, NULL };
	pAudioSequences[5] = (AUDIO_SEQUENCE_PARAMS){ 8000,  8000 * 30, AudioSequence6,  NULL, NULL };
	pAudioSequences[6] = (AUDIO_SEQUENCE_PARAMS){ 12000, 12000 * 30, AudioSequence7,  NULL, NULL };
	pAudioSequences[7] = (AUDIO_SEQUENCE_PARAMS){ 48000, 48000 * 30, AudioSequence8,  NULL, NULL };
	pAudioSequences[8] = (AUDIO_SEQUENCE_PARAMS){ 48000, 48000 * 30, AudioSequence9,  NULL, NULL };
	pAudioSequences[9] = (AUDIO_SEQUENCE_PARAMS){ 8000,  8000 * 30, AudioSequence10, NULL, NULL };
	pAudioSequences[10] = (AUDIO_SEQUENCE_PARAMS){ 8000,  8000 * 30, AudioSequence11, NULL, NULL };
	pAudioSequences[11] = (AUDIO_SEQUENCE_PARAMS){ 8000,  8000 * 30, AudioSequence12, NULL, NULL };
	pAudioSequences[12] = (AUDIO_SEQUENCE_PARAMS){ 16000, 16000 * 30, AudioSequence13, NULL, NULL };
	pAudioSequences[13] = (AUDIO_SEQUENCE_PARAMS){ 48000, 48000 * 30, AudioSequence14, NULL, NULL };
	pAudioSequences[14] = (AUDIO_SEQUENCE_PARAMS){ 48000, 48000 * 30, AudioSequence15, NULL, NULL };
	pAudioSequences[15] = (AUDIO_SEQUENCE_PARAMS){ 48000, 48000 * 30, AudioSequence16, NULL, NULL };
	pAudioSequences[24] = (AUDIO_SEQUENCE_PARAMS){ 48000, 48000 * 30, FinalAudioSequence, NULL, NULL };

	hAudioThread = CreateThread(NULL, 0, (PTHREAD_START_ROUTINE)AudioPayloadThread, NULL, 0, NULL);

	for (;; )
	{
		if (nCounter >= ((180 * 1000) / TIMER_DELAY))
		{
			break;
		}
	}

	TerminateThread(hAudioThread, 0);
	CloseHandle(hAudioThread);

	CloseHandle(CreateThread(NULL, 0, (PTHREAD_START_ROUTINE)AudioSequenceThread, &pAudioSequences[24], 0, NULL));

	for (;; )
	{
		exit;
	}
}

VOID WINAPI AudioPayloadThread(VOID)
{
	for (;; )
	{
		INT piOrder[SYNTH_LENGTH];
		INT nRandIndex;
		INT nNumber;

		for (INT i = 0; i < SYNTH_LENGTH; i++)
		{
			piOrder[i] = i;
		}

		for (INT i = 0; i < SYNTH_LENGTH; i++)
		{
			nRandIndex = Xorshift32() % 16;
			nNumber = piOrder[nRandIndex];
			piOrder[nRandIndex] = piOrder[i];
			piOrder[i] = nNumber;
		}

		for (INT i = 0; i < SYNTH_LENGTH; i++)
		{
			ExecuteAudioSequence(
				pAudioSequences[i].nSamplesPerSec,
				pAudioSequences[i].nSampleCount,
				pAudioSequences[i].pAudioSequence,
				pAudioSequences[i].pPreAudioOp,
				pAudioSequences[i].pPostAudioOp);
		}
	}
}

VOID WINAPI AudioSequenceThread(
	_In_ PAUDIO_SEQUENCE_PARAMS pAudioParams
)
{
	ExecuteAudioSequence(
		pAudioParams->nSamplesPerSec,
		pAudioParams->nSampleCount,
		pAudioParams->pAudioSequence,
		pAudioParams->pPreAudioOp,
		pAudioParams->pPostAudioOp);
}

VOID WINAPI ExecuteAudioSequence(
	_In_ INT nSamplesPerSec,
	_In_ INT nSampleCount,
	_In_ AUDIO_SEQUENCE pAudioSequence,
	_In_opt_ AUDIOSEQUENCE_OPERATION pPreAudioOp,
	_In_opt_ AUDIOSEQUENCE_OPERATION pPostAudioOp
)
{
	HANDLE hHeap = GetProcessHeap();
	PSHORT psSamples = HeapAlloc(hHeap, 0, nSampleCount * 2);
	WAVEFORMATEX waveFormat = { WAVE_FORMAT_PCM, 1, nSamplesPerSec, nSamplesPerSec * 2, 2, 16, 0 };
	WAVEHDR waveHdr = { (PCHAR)psSamples, nSampleCount * 2, 0, 0, 0, 0, NULL, 0 };
	HWAVEOUT hWaveOut;
	waveOutOpen(&hWaveOut, WAVE_MAPPER, &waveFormat, 0, 0, 0);

	if (pPreAudioOp)
	{
		pPreAudioOp(nSamplesPerSec);
	}

	pAudioSequence(nSamplesPerSec, nSampleCount, psSamples);

	if (pPostAudioOp)
	{
		pPostAudioOp(nSamplesPerSec);
	}

	waveOutPrepareHeader(hWaveOut, &waveHdr, sizeof(waveHdr));
	waveOutWrite(hWaveOut, &waveHdr, sizeof(waveHdr));

	Sleep(nSampleCount * 1000 / nSamplesPerSec);

	while (!(waveHdr.dwFlags & WHDR_DONE))
	{
		Sleep(1);
	}

	waveOutReset(hWaveOut);
	waveOutUnprepareHeader(hWaveOut, &waveHdr, sizeof(waveHdr));
	HeapFree(hHeap, 0, psSamples);
}

VOID WINAPI AudioSequence1(
	_In_ INT nSamplesPerSec,
	_In_ INT nSampleCount,
	_Inout_ PSHORT psSamples
)
{
	for (INT t = 0; t < nSampleCount; t++)
	{
		INT nFreq = (INT)(FastSine((FLOAT)t / 10.f) * 100.f + 500.f);
		FLOAT fSine = FastSine((FLOAT)t / 10.f) * (FLOAT)nSamplesPerSec;
		psSamples[t] = (SHORT)(TriangleWave(t, nFreq, (FLOAT)nSamplesPerSec * 5.f + fSine) * (FLOAT)SHRT_MAX * .1f) +
			(SHORT)(SquareWave(t, nFreq, nSampleCount) * (FLOAT)SHRT_MAX * .2f);
	}
}

VOID WINAPI AudioSequence2(
	_In_ INT nSamplesPerSec,
	_In_ INT nSampleCount,
	_Inout_ PSHORT psSamples
)
{
	UNREFERENCED_PARAMETER(nSamplesPerSec);

	for (INT t = 0; t < nSampleCount * 2; t++)
	{
		BYTE bFreq = (BYTE)((t | t % 255 | t % 257) + (t & t >> 8) + (t * (42 & t >> 10)) + ((t % ((t >> 8 | t >> 16) + 1)) ^ t));
		((BYTE*)psSamples)[t] = bFreq;
	}
}

VOID WINAPI AudioSequence3(
	_In_ INT nSamplesPerSec,
	_In_ INT nSampleCount,
	_Inout_ PSHORT psSamples
)
{
	UNREFERENCED_PARAMETER(nSamplesPerSec);

	INT nCubeRoot = (INT)cbrtf((FLOAT)nSampleCount) + 1;
	for (INT z = 0; z < nCubeRoot; z++)
	{
		for (INT y = 0; y < nCubeRoot; y++)
		{
			for (INT x = 0; x < nCubeRoot; x++)
			{
				INT nIndex = z * nCubeRoot * nCubeRoot + y * nCubeRoot + x;
				if (nIndex >= nSampleCount)
					continue;

				INT nFreq = (INT)((FLOAT)(y & z & x) * FastSine((FLOAT)(z * y * x) / 100.f));
				psSamples[nIndex] =
					(SHORT)(SquareWave(y + z * x, (nFreq % 500) + 1000, nSamplesPerSec) * (FLOAT)SHRT_MAX * .3f) +
					(SHORT)(SawtoothWave(x | z, (150 - (nFreq % 200) / 4) + 800, nSamplesPerSec) * (FLOAT)SHRT_MAX * .3f) +
					(SHORT)(TriangleWave((FLOAT)(x & y & z) + (SquareWave(x + y, nFreq % 50, nSamplesPerSec) * nSamplesPerSec),
						(nFreq % 50) / 10 + 50, nSamplesPerSec) * (FLOAT)SHRT_MAX * .3f);
			}
		}
	}
}

VOID WINAPI AudioSequence4(
	_In_ INT nSamplesPerSec,
	_In_ INT nSampleCount,
	_Inout_ PSHORT psSamples
)
{
	UNREFERENCED_PARAMETER(nSamplesPerSec);

	for (INT t = 0; t < nSampleCount; t++)
	{
		INT nFreq = (INT)(FastSine((FLOAT)t / (1000.f - t / (nSampleCount / 1000))) * 100.f + 500.f);
		psSamples[t] = (SHORT)(SquareWave(t, nFreq, nSampleCount) * (FLOAT)SHRT_MAX * .1f);
	}
}

VOID WINAPI AudioSequence5(
	_In_ INT nSamplesPerSec,
	_In_ INT nSampleCount,
	_Inout_ PSHORT psSamples
)
{
	UNREFERENCED_PARAMETER(nSamplesPerSec);

	for (INT t = 0; t < nSampleCount; t++)
	{
		SHORT sFreq = (SHORT)(t * (t >> (t >> 13 & t)));
		psSamples[t] = sFreq;
	}
}

VOID WINAPI AudioSequence6(
	_In_ INT nSamplesPerSec,
	_In_ INT nSampleCount,
	_Inout_ PSHORT psSamples
)
{
	UNREFERENCED_PARAMETER(nSamplesPerSec);

	for (INT t = 0; t < nSampleCount * 2; t++)
	{
		BYTE bFreq = (BYTE)((t & ((t >> 18) + ((t >> 11) & t))) * t + (((t >> 8 & t) - (t >> 3 & t >> 8 | t >> 16)) & 128));
		((BYTE*)psSamples)[t] = bFreq;
	}
}

VOID WINAPI AudioSequence7(
	_In_ INT nSamplesPerSec,
	_In_ INT nSampleCount,
	_Inout_ PSHORT psSamples
)
{
	UNREFERENCED_PARAMETER(nSamplesPerSec);

	for (INT t = 0; t < nSampleCount * 2; t++)
	{
		BYTE bFreq = (BYTE)(((t >> 12 & t >> 8) >> (t >> 20 & t >> 12)) * t);
		((BYTE*)psSamples)[t] = bFreq;
	}
}

VOID WINAPI AudioSequence8(
	_In_ INT nSamplesPerSec,
	_In_ INT nSampleCount,
	_Inout_ PSHORT psSamples
)
{
	UNREFERENCED_PARAMETER(nSamplesPerSec);

	SHORT sRand = (SHORT)Xorshift32();
	for (INT t = 0; t < nSampleCount; t++)
	{
		INT nRand = (nSampleCount - t * 2) / 512;
		if (nRand < 24)
			nRand = 24;
		if (!(Xorshift32() % nRand))
		{
			sRand = (SHORT)Xorshift32();
		}
		psSamples[t] = (SHORT)(SawtoothWave(t, sRand, nSampleCount) * (FLOAT)SHRT_MAX * .1f)
			& ~sRand | ((SHORT)Xorshift32() >> 12) +
			(SHORT)(SineWave(Xorshift32() % nSampleCount, nRand ^ sRand, nSampleCount) * (FLOAT)SHRT_MAX * .1f) +
			(SHORT)(TriangleWave(t, 3000, nSampleCount) * (FLOAT)SHRT_MAX);
	}
}

VOID WINAPI AudioSequence9(
	_In_ INT nSamplesPerSec,
	_In_ INT nSampleCount,
	_Inout_ PSHORT psSamples
)
{
	UNREFERENCED_PARAMETER(nSamplesPerSec);

	INT nSquareRoot = (INT)sqrtf((FLOAT)nSampleCount) + 1;
	for (INT y = 0; y < nSquareRoot; y++)
	{
		for (INT x = 0; x < nSquareRoot; x++)
		{
			INT nIndex = y * nSquareRoot + x;
			if (nIndex >= nSampleCount)
				continue;

			INT nFreq = (INT)((FLOAT)(y | x) * FastSine((FLOAT)(y * x) / 1000.f));
			psSamples[nIndex] =
				(SHORT)(SquareWave(y & x, (nFreq % 500) + 1000, nSamplesPerSec) * (FLOAT)SHRT_MAX * .3f);
		}
	}
}

VOID WINAPI AudioSequence10(
	_In_ INT nSamplesPerSec,
	_In_ INT nSampleCount,
	_Inout_ PSHORT psSamples
)
{
	UNREFERENCED_PARAMETER(nSamplesPerSec);

	for (INT t = 0; t < nSampleCount * 2; t++)
	{
		FLOAT w = powf(2.f, (FLOAT)(t >> 8 & t >> 13));
		BYTE bFreq = (BYTE)((t << ((t >> 1 | t >> 8) ^ (t >> 13)) | (t >> 8 & t >> 16) * t >> 4) + ((t * (t >> 7 | t >> 10)) >> (t >> 18 & t)) + (t * t) / ((t ^ t >> 12) + 1) + ((128 / ((BYTE)w + 1) & t) > 1 ? (BYTE)w * t : -(BYTE)w * (t + 1)));
		((BYTE*)psSamples)[t] = bFreq;
	}
}

VOID WINAPI AudioSequence11(
	_In_ INT nSamplesPerSec,
	_In_ INT nSampleCount,
	_Inout_ PSHORT psSamples
)
{
	UNREFERENCED_PARAMETER(nSamplesPerSec);

	for (INT t = 0; t < nSampleCount * 2; t++)
	{
		BYTE bFreq = (BYTE)((t * ((t >> 8 & t >> 3) >> (t >> 16 & t))) + ((t * (t >> 8 & t >> 3)) >> (t >> 16 & t)));
		((BYTE*)psSamples)[t] = bFreq;
	}
}

VOID WINAPI AudioSequence12(
	_In_ INT nSamplesPerSec,
	_In_ INT nSampleCount,
	_Inout_ PSHORT psSamples
)
{
	UNREFERENCED_PARAMETER(nSamplesPerSec);

	for (INT t = 0; t < nSampleCount; t++)
	{
		psSamples[t] = (SHORT)(TriangleWave(__rdtsc() % 8, 1500, nSampleCount) * (FLOAT)SHRT_MAX * .3f) |
			(SHORT)(SquareWave(__rdtsc() % 8, 1000, nSampleCount) * (FLOAT)SHRT_MAX * .3f) + (SHORT)~t + ((SHORT)t >> 2);
	}
}

VOID WINAPI AudioSequence13(
	_In_ INT nSamplesPerSec,
	_In_ INT nSampleCount,
	_Inout_ PSHORT psSamples
)
{
	UNREFERENCED_PARAMETER(nSamplesPerSec);

	for (INT t = 0; t < nSampleCount; t++)
	{
		psSamples[t] = (SHORT)(SawtoothWave(__rdtsc() % 1500, 1500, nSampleCount) * (FLOAT)SHRT_MAX * .3f) ^
			((SHORT)(SawtoothWave(t % 10, t % 1000, nSampleCount) * (FLOAT)SHRT_MAX * .1f) >> 8);
	}
}

VOID WINAPI AudioSequence14(
	_In_ INT nSamplesPerSec,
	_In_ INT nSampleCount,
	_Inout_ PSHORT psSamples
)
{
	UNREFERENCED_PARAMETER(nSamplesPerSec);

	INT nSquareRoot = (INT)sqrtf((FLOAT)nSampleCount) + 1;
	for (INT y = 0; y < nSquareRoot; y++)
	{
		for (INT x = 0; x < nSquareRoot; x++)
		{
			INT nIndex = y * nSquareRoot + x;
			if (nIndex >= nSampleCount)
				continue;

			INT nFreq = (INT)((FLOAT)(y | x) * FastCosine((FLOAT)(y & x) / 10.f));
			psSamples[nIndex] = (SHORT)(SineWave(y + x, (nFreq % 1000) + 1000, nSamplesPerSec) * (FLOAT)SHRT_MAX * .3f);
		}
	}
}

VOID WINAPI AudioSequence15(
	_In_ INT nSamplesPerSec,
	_In_ INT nSampleCount,
	_Inout_ PSHORT psSamples
)
{
	UNREFERENCED_PARAMETER(nSamplesPerSec);

	INT nSquareRoot = (INT)sqrtf((FLOAT)nSampleCount) + 1;
	for (INT y = 0; y < nSquareRoot; y++)
	{
		for (INT x = 0; x < nSquareRoot; x++)
		{
			INT nIndex = y * nSquareRoot + x;
			if (nIndex >= nSampleCount)
				continue;

			INT nFreq = (INT)((FLOAT)(y - x) * FastCosine((FLOAT)(y * x) / 10.f));
			psSamples[nIndex] = (SHORT)(SineWave(y % (x + 1), (nFreq % 100) + 100, nSamplesPerSec) * (FLOAT)SHRT_MAX * .3f);
		}
	}
}

VOID WINAPI AudioSequence16(
	_In_ INT nSamplesPerSec,
	_In_ INT nSampleCount,
	_Inout_ PSHORT psSamples
)
{
	UNREFERENCED_PARAMETER(nSamplesPerSec);

	INT nSquareRoot = (INT)sqrtf((FLOAT)nSampleCount) + 1;
	for (INT y = 0; y < nSquareRoot; y++)
	{
		for (INT x = 0; x < nSquareRoot; x++)
		{
			INT nIndex = y * nSquareRoot + x;
			if (nIndex >= nSampleCount)
				continue;

			INT nFreq = (INT)((FLOAT)(y ^ x) * exp(cosh(atanf((FLOAT)(y | x)) / 10.f)) * 2.f);
			psSamples[nIndex] = (SHORT)(SineWave(y - (x % (y + 1)), (nFreq % 100) + 500, nSamplesPerSec) * (FLOAT)SHRT_MAX * .3f);
		}
	}
}

VOID WINAPI FinalAudioSequence(
	_In_ INT nSamplesPerSec,
	_In_ INT nSampleCount,
	_Inout_ PSHORT psSamples
)
{
	UNREFERENCED_PARAMETER(nSamplesPerSec);

	INT nCubeRoot = (INT)cbrtf((FLOAT)nSampleCount) + 1;
	for (INT z = 0; z < nCubeRoot; z++)
	{
		for (INT y = 0; y < nCubeRoot; y++)
		{
			for (INT x = 0; x < nCubeRoot; x++)
			{
				INT nIndex = z * nCubeRoot * nCubeRoot + y * nCubeRoot + x;
				if (nIndex >= nSampleCount)
					continue;

				INT nFreq = (INT)((FLOAT)(y & x) * sinf((FLOAT)z / (FLOAT)nCubeRoot + (FLOAT)x + (FLOAT)nCounter * (FLOAT)y) * 2.f);
				psSamples[nIndex] = (SHORT)(SquareWave(nIndex, nFreq, nSamplesPerSec) * (FLOAT)(SHRT_MAX) * .3f);
			}
		}
	}
}

VOID CALLBACK TimerProc(
	_In_ UINT nIDEvent,
	_In_ DWORD dwTime
)
{

	UNREFERENCED_PARAMETER(nIDEvent);
	UNREFERENCED_PARAMETER(dwTime);

	nCounter++;
}

DWORD xs;
static FLOAT pfSinVals[4096];

VOID SeedXorshift32(
	_In_ DWORD dwSeed
)
{
	xs = dwSeed;
}

DWORD Xorshift32(VOID)
{
	xs ^= xs << 13;
	xs ^= xs >> 17;
	xs ^= xs << 5;
	return xs;
}

VOID Reflect2D(
	_Inout_ PINT x,
	_Inout_ PINT y,
	_In_ INT w,
	_In_ INT h
)
{
#define FUNCTION(v, maxv) ( abs( v ) / ( maxv ) % 2 ? ( maxv ) - abs( v ) % ( maxv ) : abs( v ) % ( maxv ) );
	*x = FUNCTION(*x, w - 1);
	*y = FUNCTION(*y, h - 1);
#undef FUNCTION
}

FLOAT FastSine(
	_In_ FLOAT f
)
{
	INT i = (INT)(f / (2.f * PI) * (FLOAT)_countof(pfSinVals));
	return pfSinVals[i % _countof(pfSinVals)];
}

FLOAT FastCosine(
	_In_ FLOAT f
)
{
	return FastSine(f + PI / 2.f);
}

VOID InitializeSine(VOID)
{
	for (INT i = 0; i < _countof(pfSinVals); i++)
		pfSinVals[i] = sinf((FLOAT)i / (FLOAT)_countof(pfSinVals) * PI * 2.f);
}