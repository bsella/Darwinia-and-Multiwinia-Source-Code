use std::{
    ffi::{CStr, OsStr, c_char},
    fs::File,
    os::unix::ffi::OsStrExt,
    path::{Path, PathBuf},
};

use symphonia::core::{
    errors::Error,
    formats::{TrackType, probe::Hint},
    io::MediaSourceStream,
};

type Sample = i16;

struct CachedSample {
    num_channels: usize,
    frequency: u32,
    samples: Vec<Sample>,
}

impl CachedSample {
    fn new(filename: &Path) -> Self {
        let file = File::open(filename).expect("File not found");

        let stream = MediaSourceStream::new(Box::new(file), Default::default());

        // Probe the media source.
        // Use the default options for metadata and format readers.
        let mut format = symphonia::default::get_probe()
            .probe(&Hint::new(), stream, Default::default(), Default::default())
            .expect("unsupported format");

        // Find the first audio track with a known (decodeable) codec.
        let track = format
            .default_track(TrackType::Audio)
            .expect("no audio track");

        // Create a decoder for the track.
        // Use the default options for the decoder.
        let mut decoder = symphonia::default::get_codecs()
            .make_audio_decoder(
                track
                    .codec_params
                    .as_ref()
                    .expect("codec parameters missing")
                    .audio()
                    .unwrap(),
                &Default::default(),
            )
            .expect("unsupported codec");

        // Store the track identifier, it will be used to filter packets.
        let track_id = track.id;

        let mut samples = Vec::<Sample>::new();
        let mut specs = None::<(u32, usize)>;

        // The decode loop.
        loop {
            // Get the next packet from the media format.
            let packet = match format.next_packet() {
                Ok(Some(packet)) => packet,
                Ok(None) => {
                    // Reached the end of the stream.
                    break;
                }
                Err(Error::ResetRequired) => {
                    // The track list has been changed. Re-examine it and create a new set of decoders,
                    // then restart the decode loop. This is an advanced feature and it is not
                    // unreasonable to consider this "the end." As of v0.5.0, the only usage of this is
                    // for chained OGG physical streams.
                    unimplemented!();
                }
                Err(err) => {
                    // A unrecoverable error occurred, halt decoding.
                    panic!("{}", err);
                }
            };

            // Consume any new metadata that has been read since the last packet.
            while !format.metadata().is_latest() {
                // Pop the old head of the metadata queue.
                format.metadata().pop();

                // Consume the new metadata at the head of the metadata queue.
            }

            // If the packet does not belong to the selected track, skip over it.
            if packet.track_id != track_id {
                continue;
            }

            // Decode the packet into audio samples.
            match decoder.decode(&packet) {
                Ok(decoded) => {
                    // Override the specs. We will be getting the specs of the last packet
                    specs = Some((decoded.spec().rate(), decoded.spec().channels().count()));

                    let num_samples = decoded.samples_interleaved();
                    let old_len = samples.len();
                    samples.resize(old_len + num_samples, Default::default());
                    decoded.copy_to_slice_interleaved(&mut samples[old_len..]);

                    // Consume the decoded audio samples (see below).
                }
                Err(Error::IoError(_)) => {
                    // The packet failed to decode due to an IO error, skip the packet.
                    continue;
                }
                Err(Error::DecodeError(_)) => {
                    // The packet failed to decode due to invalid data, skip the packet.
                    continue;
                }
                Err(err) => {
                    // An unrecoverable error occurred, halt decoding.
                    panic!("{}", err);
                }
            }
        }

        let (frequency, num_channels) = specs.expect("Specs not found");

        CachedSample {
            frequency,
            num_channels,
            samples,
        }
    }

    fn read_samples(&self, offset: usize, samples: &mut [Sample]) -> std::io::Result<usize> {
        if offset + samples.len() > self.samples.len() {
            return Err(std::io::Error::new(
                std::io::ErrorKind::Other,
                "Out of boundary",
            ));
        }

        samples.copy_from_slice(&self.samples[offset..offset + samples.len()]);

        Ok(samples.len())
    }
}

/////////

#[unsafe(no_mangle)]
extern "C" fn darw_CreateCachedSample(filename: *const c_char) -> *mut CachedSample {
    let slice = unsafe { CStr::from_ptr(filename) };
    let osstr = OsStr::from_bytes(slice.to_bytes());
    let path: &Path = osstr.as_ref();

    let mut sound_path = PathBuf::from("data/sounds");

    sound_path.push(path);
    sound_path.add_extension("ogg");

    Box::leak(Box::new(CachedSample::new(&sound_path)))
}

#[unsafe(no_mangle)]
extern "C" fn darw_DeleteCachedSample(cached_sample: *mut CachedSample) {
    if !cached_sample.is_null() {
        drop(unsafe { Box::from_raw(cached_sample) });
    }
}

#[unsafe(no_mangle)]
extern "C" fn darw_CachedSampleRead(
    cached_sample: *const CachedSample,
    data: *mut i16,
    start_sample: u32,
    num_samples: u32,
) {
    let total_num_samples = darw_CachedSampleNumSamples(cached_sample);

    let num_samples_to_read = (total_num_samples - start_sample).min(num_samples);

    let samples = unsafe { std::slice::from_raw_parts_mut(data, num_samples_to_read as usize) };
    let _ = unsafe { &(*cached_sample) }.read_samples(start_sample as usize, samples);
}

#[unsafe(no_mangle)]
extern "C" fn darw_CachedSampleNumSamples(cached_sample: *const CachedSample) -> u32 {
    unsafe { (*cached_sample).samples.len() as u32 }
}

#[unsafe(no_mangle)]
extern "C" fn darw_CachedSampleNumChannels(cached_sample: *const CachedSample) -> u32 {
    unsafe { (*cached_sample).num_channels as u32 }
}

#[unsafe(no_mangle)]
extern "C" fn darw_CachedSampleNumFreq(cached_sample: *const CachedSample) -> u32 {
    unsafe { (*cached_sample).frequency }
}
