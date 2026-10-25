{-# LANGUAGE ScopedTypeVariables #-}
{- A block comment before the module,
   over two lines, with {- a nested one -} inside. -}
module Main where

{- Between the header and the imports. -}
import Data.List (sort)
{- Between two imports. -}
import Data.Maybe (fromMaybe)

-- A line comment.
{-
{-# INLINE ignored #-}
ignored :: Int
ignored = 1
-}
{-# INLINE main #-}
main :: IO ()
main = do
  print (sort [3, 1, 2]) {- trailing -}
  {- inside the block -}
{- at column zero, inside the block -}
        {- deeper than the block -}
  print (fromMaybe 0 Nothing)
{- after the last declaration -}
