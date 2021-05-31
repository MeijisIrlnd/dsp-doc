#ifndef SIGNALSMITH_DSP_SPECTRAL_H
#define SIGNALSMITH_DSP_SPECTRAL_H

#include "./common.h"
#include "./fft.h"
#include "./delay.h"

#include <cmath>

namespace signalsmith {
namespace spectral {
	/**	@defgroup Spectral Spectral Processing
		@brief Tools for frequency-domain manipulation of audio signals
		
		@{
		@file
	*/
	
	/** @brief An FFT with built-in windowing and round-trip scaling
	
		This uses a Modified Real FFT, which applies half-bin shift before the transform.  The result therefore has `N/2` bins, centred at the frequencies: `(i + 0.5)/N`.
		
		This avoids the awkward (real-valued) bands for DC-offset and Nyquist.
	 */
	template<typename Sample>
	class WindowedFFT {
		using MRFFT = signalsmith::ModifiedRealFFT<Sample>;
		using Complex = std::complex<Sample>;
		MRFFT mrfft{2};

		std::vector<Sample> window;
		std::vector<Sample> timeBuffer;
		std::vector<Complex> freqBuffer;
	public:
		/// Returns a fast FFT size <= `size`
		static int sizeMaximum(int size, int divisor=1) {
			return MRFFT::sizeMinimum(size/divisor)*divisor;
		}
		/// Returns a fast FFT size >= `size`
		static int sizeMinimum(int size, int divisor=1) {
			return MRFFT::sizeMinimum(1 + (size - 1)/divisor)*divisor;
		}

		WindowedFFT() {}
		WindowedFFT(int size) {
			setSize(size);
		}
		template<class WindowFn>
		WindowedFFT(int size, WindowFn fn, Sample windowOffset=0.5) {
			setSize(size, fn, windowOffset);
		}

		/// Sets the size, returning the window for modification (initially all 1s)
		std::vector<Sample> & setSizeWindow(int size) {
			mrfft.setSize(size);
			window.resize(size, 1);
			timeBuffer.resize(size);
			freqBuffer.resize(size);
			return window;
		}
		/// Sets the FFT size, with a user-defined functor for the window
		template<class WindowFn>
		void setSize(int size, WindowFn fn, Sample windowOffset=0.5) {
			setSizeWindow(size);
		
			Sample invSize = 1/(Sample)size;
			for (int i = 0; i < size; ++i) {
				Sample r = (i + windowOffset)*invSize;
				window[i] = fn(r);
			}
		}
		/// Sets the size (using the default Blackman-Harris window)
		void setSize(int size) {
			setSize(size, [](double x) {
				double phase = 2*M_PI*x;
				// Blackman-Harris
				return 0.35875 + 0.48829*std::cos(phase) + 0.14128*std::cos(phase*2) + 0.1168*std::cos(phase*3);
			});
		}
		
		/// Performs an FFT (with windowing)
		template<class Input, class Output>
		void fft(Input &&input, Output &&output) {
			struct WindowedInput {
				const Input &input;
				std::vector<Sample> &window;
				Sample operator [](int i) {
					return input[i]*window[i];
				}
			};
		
			mrfft.fft(WindowedInput{input, window}, output);
		}

		/// Inverse FFT, with windowing and 1/N scaling
		template<class Input, class Output>
		void ifft(Input &&input, Output &&output) {
			mrfft.ifft(input, output);
			int size = mrfft.size();
			Sample norm = 1/(Sample)size;
			for (int i = 0; i < size; ++i) {
				output[i] *= norm*window[i];
			}
		}
	};
	
	/** STFT synthesis/analysis/processing, built on a `MultiBuffer`.
 
		Any window length and block interval is supported, but the FFT size may be rounded up to a faster size (by zero-padding).  It uses a Kaiser window modified for perfect-reconstruction, with shape chosen for almost-optimal aliasing (band-separation) performance.
		
		\diagram{stft-aliasing-simulated.svg,Simulated bad-case aliasing (random phase-shift for each band) for overlapping ratios}

		There is a "latest valid index", and you can read the output up to one `historyLength` behind this (see `.resize()`).  You can read up to one window-length _ahead_ to get partially-summed future output.
		
		\diagram{stft-buffer-validity.svg}
		
		You move the valid index along using `.ensureValid()`, passing in a functor which provides spectra (using `.analyse()` and/or direct modification through `.spectrum(c)`):

		\code
			void processSample(...) {
				stft.ensureValid([&](int) {
					// Here, we introduce (1 - windowSize) of latency
					stft.analyse(inputBuffer.view(1 - windowSize))
				});
				// read as a MultiBuffer
				auto result = stft.at(0);
				++stft; // also moves the latest valid index
			}

			void processBlock(...) {
				// assuming `historyLength` == blockSize
				stft.ensureValid(blockSize, [&](int blockStartIndex) {
					int inputStart = blockStartIndex + (1 - windowSize);
					stft.analyse(inputBuffer.view(inputStart));
				});
				auto earliestValid = stft.at(0);
				auto latestValid = stft.at(blockSize);
				stft += blockSize;
			}
		\endcode
		
		The index passed to this functor will be greater than the previous valid index, and `<=` the index you pass in.  Therefore, if you call `.ensureValid()` every sample, it can only ever be `0`.
	*/
	template<typename Sample>
	class STFT : public signalsmith::delay::MultiBuffer<Sample> {
		using Super = signalsmith::delay::MultiBuffer<Sample>;
		using Complex = std::complex<Sample>;

		int channels = 0, _windowSize = 0, fftSize = 0, interval = 1;
		int validUntilIndex = 0;
		WindowedFFT<Sample> fft;

		class MultiSpectrum {
			int channels, stride;
			std::vector<Complex> spectrumBuffer;
		public:
			MultiSpectrum() : MultiSpectrum(0, 0) {}
			MultiSpectrum(int channels, int bands) : channels(channels), stride(bands), spectrumBuffer(channels*bands, 0) {}
			
			void resize(int channels, int bands) {
				this->channels = channels;
				this->stride = bands;
				spectrumBuffer.assign(channels*bands, 0);
			}
			
			void reset() {
				spectrumBuffer.assign(spectrumBuffer.size(), 0);
			}

			Complex * operator [](int channel) {
				return spectrumBuffer.data() + channel*stride;
			}
			const Complex * operator [](int channel) const {
				return spectrumBuffer.data() + channel*stride;
			}
		};
		std::vector<Sample> timeBuffer;

		void resizeInternal(int channels, int windowSize, int interval, int historyLength) {
			Super::resize(channels, windowSize);
			int fftSize = fft.sizeMinimum(windowSize);
			
			this->channels = channels;
			_windowSize = windowSize;
			this->fftSize = fftSize;
			this->interval = interval;
			validUntilIndex = -1;

			auto &window = fft.setSizeWindow(fftSize);
			signalsmith::windows::fillKaiserStft(window, windowSize, interval);
			// TODO: fill extra bits of an input buffer with NaN/Infinity, to break this, and then fix by adding zero-padding to WindowedFFT (as opposed to zero-valued window sections)
			for (int i = windowSize; i < fftSize; ++i) {
				window[i] = 0;
			}

			Super::resize(channels,
				windowSize /* for output summing */
				+ interval /* so we can read `windowSize` ahead (we'll be at most `interval-1` from the most recent block */
				+ historyLength);
			spectrum.resize(channels, fftSize/2);
			timeBuffer.resize(fftSize);
		}
	public:
		using Spectrum = MultiSpectrum;
		Spectrum spectrum;

		STFT() {}
		/// Parameters passed straight to `.resize()`
		STFT(int channels, int windowSize, int interval, int historyLength=0) {
			resize(channels, windowSize, interval, historyLength);
		}

		/// Sets the channel-count, FFT size and interval.
		void resize(int channels, int windowSize, int interval, int historyLength=0) {
			resizeInternal(channels, windowSize, interval, historyLength);
		}
		
		int windowSize() {
			return _windowSize;
		}
		
		/// Resets everything - since we clear the output sum, it will take `windowSize` samples to get proper output.
		void reset() {
			Super::reset();
			spectrum.reset();
			validUntilIndex = -1;
		}
		
		/** Generates valid output up to the specified index (or 0), using the callback as many times as needed.
			
			The callback should be a functor accepting a single integer argument, which is the index for which a spectrum is required.
			
			The block created from these spectra will start at this index in the output, plus `.latency()`.
		*/
		template<class AnalysisFn>
		void ensureValid(int i, AnalysisFn fn) {
			while (validUntilIndex < i) {
				int blockIndex = validUntilIndex + 1;
				fn(blockIndex);

				auto output = this->view(blockIndex);
				for (int c = 0; c < channels; ++c) {
					auto channel = output[c];

					// Clear out the future sum, a window-length and an interval ahead
					for (int i = _windowSize; i < _windowSize + interval; ++i) {
						channel[i] = 0;
					}

					// Add in the IFFT'd result
					fft.ifft(spectrum[c], timeBuffer);
					for (int i = 0; i < _windowSize; ++i) {
						channel[i] += timeBuffer[i];
					}
				}
				validUntilIndex += interval;
			}
		}
		/// The same as above, assuming index 0
		template<class AnalysisFn>
		void ensureValid(AnalysisFn fn) {
			return ensureValid(0, fn);
		}
		
		/** Analyse a multi-channel input, for any type where `data[channel][index]` returns samples
 
		Results can be read/edited using `.spectrum()`. */
		template<class Data>
		void analyse(Data &&data) {
			for (int c = 0; c < channels; ++c) {
				fft.fft(data[c], spectrum[c]);
			}
		}
		template<class Data>
		void analyse(int c, Data &&data) {
			fft.fft(data, spectrum[c]);
		}

		int bands() const {
			return fftSize/2;
		}

		/** Internal latency (between the block-index requested in `.ensureValid()` and its position in the output)
 
		Currently unused, but it's in here to allow for a future implementation which spreads the FFT calculations out across each interval.*/
		int latency() {
			return 0;
		}
		
		// @name Shift the underlying buffer (moving the "valid" index accordingly)
		// @{
		STFT & operator ++() {
			Super::operator ++();
			validUntilIndex--;
			return *this;
		}
		STFT & operator +=(int i) {
			Super::operator +=(i);
			validUntilIndex -= i;
			return *this;
		}
		STFT & operator --() {
			Super::operator --();
			validUntilIndex++;
			return *this;
		}
		STFT & operator -=(int i) {
			Super::operator -=(i);
			validUntilIndex += i;
			return *this;
		}
		// @}

		typename Super::MutableView operator ++(int postIncrement) {
			auto result = Super::operator ++(postIncrement);
			validUntilIndex--;
			return result;
		}
		typename Super::MutableView operator --(int postIncrement) {
			auto result = Super::operator --(postIncrement);
			validUntilIndex++;
			return result;
		}

	};


/** @} */
}} // signalsmith::spectral::
#endif // include guard
