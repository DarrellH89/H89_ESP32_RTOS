
(************ CrcCalc ****************)
function CrcCalc( crc: integer; data: byte):integer;
  var i :integer;

  begin
  crc := crc xor integer(data) shl 8;
  for i := 1 to 8 do
      begin
      if (crc and $8000) = $8000   then
         begin
          crc := (crc shl 1) xor $1021;
          end
        else
          crc := crc shl 1;
     end;
  CrcCalc := crc ;
  end;



  (*********************** Hex Out *******************)

  function HexOut;
    const
        hex : str20  = '0123456789ABCDEF';
    var
        temp : str4 ;
        j : integer;

    begin
    temp := '';
    for j := 4 downto 1 do
        begin
        temp[j] := hex[ (num and $f) + 1 ];
        num := num shr 4;
        end;
    temp[0] := chr(4);
    j := 1;
    while (temp[j] = '0') and (j <= 2) do
        begin
        temp[j] := ' ';
        j := j + 1;
        end;
    HexOut := temp;
    end;

{********* Send Command ************************}
procedure SendCmd( cmd: byte );
  var
    result : boolean;
    begin
    Port[fCmd] := 1;
    result := SendData(cmd);
    result := SendData(5);
    result := SendData(18);
    result := SendData(44);
    {}
    end;

{****************** debug ********************}
procedure debug;
    var
       ans: char;
       err, data: byte ;
       j: integer;
    begin
dataCnt := 0;
error := 0;


Writeln('Command Port = :', HexOut(fCmd));
repeat
    cnt :=0;
    Writeln('Sending Command 1');
    sendCmd(1);
{Read(ans);
{}
    error := 0;
    j := 1;
    for cnt := 1 to 4 do
        begin
        if ReadData(data) then
           begin
           buf[j] := data;
           j := j + 1;
           end
         else
           WriteLn('Data Read error');
        end;
    for cnt := 1 to j-1 do
        begin
        Writeln('Data: ', buf[cnt]);
        end;
    Writeln('Send Command again?');
    Read(ans);
    ans := UpCase(ans);
    Writeln;
    until ans <> 'Y';
    end;

(******************* fileList ******************)

function fileList: integer ;
const
     buffSize = 1024;
var
    get: byte;
    len, ptr, err, j: integer;
    buff: array [1..buffSize] of char;
    result : boolean;
begin
WriteLn;
Port[fCmd] := 1;
result := SendData($10);
len := 0;
ptr := 1;
err := 0;
get := 1;
repeat
    if ReadData(get) then
       begin
{       Write( get,' ');
{       }
       buff[ptr] := chr(get);
       ptr := ptr + 1;
       if ptr > buffSize then
          begin
          for j := 1 to buffSize do
            Write(buff[j]);
          len := len + ptr -1;
          ptr := 1;
          end;
       if get = 10 then
          begin
          buff[ptr] := chr($d);
          ptr := ptr + 1;
          end;
       end
     else
       begin
       if err mod 10 = 0 then
          WriteLn('File List Read Error ', char(get));
       err := err + 1;
       if err > 200 then
          begin
          get := 0;
          WriteLn('Buf ptr: ', ptr,' data: ', buff[ptr]);
          end;
       end;
    until (get = 0) or (len > buffSize * 2);
len := len + ptr -1;
if ptr > 0 then
    for j := 1 to ptr-1 do
        Write(buff[j]);
WriteLn;
Writeln('Len: ',len);
fileList := len;
end;
