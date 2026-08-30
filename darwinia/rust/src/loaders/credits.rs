use crate::loaders::Loader;

pub struct CreditsLoader {}

impl Loader for CreditsLoader {
    const NAME: &std::ffi::CStr = c"Credits";
    const CHANCE_OF_SPAWNING : u32 = 10;
}
