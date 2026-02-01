c. % Hello!
% <- constant
       % <- spell
{
% <- punctuation.bracket
  [
  % <- punctuation.bracket
    {
    % <- punctuation.bracket
      true,
      % <- boolean
      %   ^ punctuation.delimiter
      4,
      % <- number
       %<- punctuation.delimiter
      cat,
      % <- constant
      %  ^ punctuation.delimiter
      -0.5e-2,
      % <- number.float
      %      ^ punctuation.delimiter
      log
      % <- constant
    }
    % <- punctuation.bracket
  ]
  % <- punctuation.bracket
}. /* Hi
         there! */
      % ^ spell

function(a,c,a,b,c).
% <- function.call
%       ^ punctuation.bracket
%        ^ constant
%         ^ punctuation.delimiter
%                 ^ punctuation.bracket
%                  ^ punctuation.delimiter
