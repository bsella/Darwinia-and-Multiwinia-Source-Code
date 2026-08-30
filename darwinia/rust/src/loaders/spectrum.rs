use crate::loaders::Loader;

pub struct SpectrumLoader {}

impl Loader for SpectrumLoader {
    const NAME: &std::ffi::CStr = c"Spectrum";
    const CHANCE_OF_SPAWNING : u32 = 10;
}
