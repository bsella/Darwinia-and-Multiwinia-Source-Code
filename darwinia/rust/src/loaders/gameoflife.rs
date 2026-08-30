use crate::loaders::Loader;

pub struct GameOfLifeLoader {}
pub struct GameOfLifeGlowLoader {}

impl Loader for GameOfLifeLoader {
    const NAME: &std::ffi::CStr = c"GameOfLife";
    const CHANCE_OF_SPAWNING : u32 = 10;
}

impl Loader for GameOfLifeGlowLoader {
    const NAME: &std::ffi::CStr = c"GameOfLife_Glow";
    const CHANCE_OF_SPAWNING : u32 = 5;
}