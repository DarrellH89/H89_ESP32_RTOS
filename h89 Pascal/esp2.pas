program esp;
{ comments}
type
    str20 = string[20];
    str4 = string[4] ;
const
     fData = $7c;
     fCmd = $7e;
     fStat = $7d;
     SOH = 1;
     ACK = 6;
     NAK = $15;
     EOT = 4;
var cnt : integer;
    dataCnt : integer;
    temp : integer;
    data: array[1..10] of integer;
    error: integer;
var status : byte;
    buf: array[1..500] of char;

function HexOut(num:integer): str4; forward;

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



  (*********** pow **************)
  function pow( one, two: byte): integer ;
  var
     result: integer;
  begin
  result := 1;
  if two > 0 then
     repeat
         result := result * one;
         two := two -1 ;
         until two = 0;
  pow :=  result;

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

(************** getStatus: byte ;            ***********)
function getStatus: byte ;
    begin
    getStatus := Port[fStat] and $3;
{    Writeln(HexOut(Port[fStat]));
{}
    end;

{************** Send Data ****************}
procedure SendData( data : byte );
    var
        timeout, cnt: integer ;
        status : byte;
    begin
    cnt := 0;
    timeout := 200;
    repeat
      status := getStatus;
      cnt := cnt +1;
{      Writeln('Status: ', HexOut(status));
{}
      until (status = 2) or (status = 0) or (cnt > timeout);
    if cnt > timeout then
        Writeln('Send Data Timeout error')
      else
        Port[fData] := data;
{
Writeln('Data: ', data,' Send Data Status: ',getStatus, ' Cnt: ', cnt);
}
    end;


{************** Read Data ****************}
function ReadData: byte;
    var
        timeout, cnt : integer;
        status, data : byte;
    begin
    cnt := 0;
    data := -1;
    timeout := 500;
    repeat
      status := getStatus;
      cnt := cnt +1;
{      Writeln('Status: ', HexOut(status));
}
      until (status = 1) or (cnt > timeout);
    if cnt > timeout then
        error := error + 1
      else
        data := Port[fData];
    ReadData := data;
    end;

{********* Send Command ************************}
procedure SendCmd( cmd: byte );
    begin
    Port[fCmd] := 1;
    SendData(cmd);
    SendData(5);
    SendData(18);
    SendData(44);
    {}
    end;

{****************** debug ********************}
procedure debug;
    var     ans: char;
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
    for cnt := 1 to 4 do
        begin
        data[cnt] := ReadData;
        end;
    for cnt := 1 to 4 do
        begin
        Writeln('Data: ', data[cnt]);
        end;
    Writeln('Send Command again?');
    Read(ans);
    ans := UpCase(ans);
    Writeln;
    until ans <> 'Y';
    end;

(******************* fileList ******************)

function fileList: integer ;
var
    get: byte;
    len: integer;

begin
WriteLn;
Port[fCmd] := 1;
SendData($10);
len := 0;
repeat
    get := ReadData;
    if get > 0 then
       begin
       buf[len] := chr(get);
       len := len + 1;
       end;
    if get = 10 then
        begin
        buf[len] := chr($d);
        len := len + 1;
        end;
    until (get = 0) or (len = 500);
Writeln('Len: ',len);
fileList := len;
end;
(************** send File ***************)
procedure sendFile ;

const
     buffSize =1024;
var
    j, fsize: integer;
    fname: str20;
    fp: file;
    buffer: array[0..buffSize] of byte;
    bptr, recs: integer;
    snum, flagEOT, errors, crc, nbytes, transize : integer;
    i, temp : integer;
    data: byte;
    label done;

    begin
    Writeln;
    Write('Enter File to Send: ');
    ReadLn(fname);
   Writeln('Sending: ', fname);
 {}
    Assign(fp, fname);
    {$I-}
    Reset(fp);      (* test if file exists *)
    {$I+}
    if IOresult <> 0 then
       begin
       Writeln('Can''t find file: ',fname);
       goto done;
       end;
    fsize := FileSize(fp)*128;
    WriteLn(' File Size: ', fsize);

    Writeln('Sending File: ', fname);
    Port[fCmd] := 1;
    SendData($30);
    for j := 1 to length(fname) do
        SendData(byte(fname[j]));
    SendData(0);
    data := ReadData;
    BlockRead(fp,buffer, buffSize div 128,recs);

    bptr :=0;
    (* send file *)
    snum :=1;
    transize := 128;
    errors := 0;
    bptr :=0;
    nbytes := recs * 128;
    flagEOT := 0;
    if nbytes < buffSize then
       flagEOT := 1;
    if nbytes = 0 then
       begin
       Writeln('Empty File');
       goto done;
       end;
    Writeln('Sending File');
    if data = byte('C') then
       Writeln('Got CRC request: ', char(data))
     else
       begin
       Writeln('Wrong byte: ', data);
       goto done;
       end;
    repeat
{          Writeln('Sending Sector: ', snum);
}
          if KeyPressed then goto done;
{          Writeln('Sending SOH: ', SOH);
}
          SendData(SOH);
{          Writeln('Status: ',getStatus);
}
          SendData(snum);
{          Writeln('Status: ',getStatus);
}
          SendData(not snum);
{          WriteLn('Complement total: ',snum + not snum);
          Writeln('Status: ',getStatus);
}
          crc := 0;
          for j := 1 to transize do
              begin
              SendData(buffer[bptr]);
              crc := CrcCalc(crc, buffer[bptr]);
              bptr := bptr + 1;
              end;
{           Writeln('CRC ',HexOut(crc));
}
           SendData((crc shr 8) and $00ff);
           SendData(crc and $ff);
           data := ReadData;

           if data = ACK then
              begin
              snum := snum + 1;
              if bptr = nbytes then
                  if flagEOT = 0 then
                     begin
                     BlockRead(fp, buffer, buffSize div 128, recs);
                     nbytes := recs * 128;
                     bptr := 0;
                     if nbytes < buffsize then
                         flagEOT := 1;
                     end
                   else
                     nbytes := 0;
             end
          else
              begin
              errors := errors + 1;
              if data = NAK then
                 WriteLn('Error: Bad sector # ',snum)
               else
                 WriteLn('Error: Garbled ACK # ', snum);
              end;
          until (nbytes = 0) and (errors < 8) ;
    SendData( EOT);
    data := ReadData;
{Writeln('data: ', data);
}
    if data <> 6 then
       Writeln('Error: File transmission end error');
    if errors < 8 then
       WriteLn('Transfer Complete')
     else
       WriteLn('Abort transfer');
    done:
    Close(fp);
    end;
{}
(************** test File ***************)
procedure testFile ;

const
     buffSize =1024;
var
    j, fsize: integer;
    fname: str20;
    fp: file;
    buffer: array[0..buffSize] of byte;
    bptr, recs: integer;
    snum, flagEOT, errors, crc, nbytes, transize : integer;
    i, temp : integer;
    data: byte;

    begin
    Writeln;
    Write('Enter File to Send: ');
    fname := 'esp1.pas';
    Writeln('Sending: ', fname);
    Assign(fp, fname);
    {$I-}
    Reset(fp);      (* test if file exists *)
    {$I+}
    if IOresult <> 0 then
       begin
       Writeln('Can''t find file: ',fname);
       end;
    fsize := FileSize(fp)*128;
    WriteLn(' File Size: ', fsize);
    repeat
          BlockRead(fp, buffer, buffSize div 128, recs);
          nbytes := recs * 128 - 1;
          for j := 0 to nbytes do
              Write(char(buffer[j]));
          until recs = 0;
    Close(fp);
  end;

(******************* Main ******************)
var cmd: char;
    last: integer;
    j : byte;
begin
WriteLn('Welcome to ESP32 Test 2');
{
last := 0;
for j := 0 to 9 do
    begin
    last := CrcCalc(last, j);
    Writeln('crc: ',HexOut(last));
    end;
Writeln('CRC test ', HexOut(last));
}
repeat
    Writeln;
    WriteLn('Enter Command to Run (0 to end)');
    Writeln('1 : Debug');
    WriteLn('2 : Get File List');
    WriteLn('3 : Send File');
    Writeln('4 : Write 1,2,3,4');
    Writeln('X: Reset ESP32');

    Read(cmd);
    case cmd of
        '1': begin
             debug;
             end;
        '2': begin
             last := fileList;
             Writeln('Bytes: ', last);
             for j := 0 to last do
                 Write(buf[j]);
             Writeln;
             end;
        '3': begin
             sendFile;
             end;
        '4': begin
             Port[$7c] :=1;
             Port[$7c] :=2;
             Port[$7c] :=3;
             Port[$7c] :=4;
             end;
        'X': begin
             Port[fCmd] := 1;
             Port[fCmd] := 1;
             end;

        '0':
         else begin
              Writeln('Invalid Command');
              end;
         end;
    until cmd = '0' ;
end.

