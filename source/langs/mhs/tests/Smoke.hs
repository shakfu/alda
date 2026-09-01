-- End-to-end smoke test fixture for the embedded MicroHs runtime.
--
-- Importing Music proves the embedded music package resolves; the arithmetic
-- proves the base package compiles and runs. Kept pure: no MIDI is emitted, so
-- the test needs no audio device.
module Smoke(main) where
import Music(quarter, c4)

main :: IO ()
main = putStrLn ("psnd-mhs-smoke " ++ show (sum [1..10::Int]) ++
                 " " ++ show c4 ++ " " ++ show quarter)
