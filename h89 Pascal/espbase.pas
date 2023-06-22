
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
    delay(20);
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


(************** send File ***************)
procedure sendFile ;

const
     buffSize =1024;
var
    j, fsize: integer;
    fname: str20;
    fp: file;
    buffer: array[0..buffSize] of byte;
    bptrS, bptr, recs: integer;
    snum, flagEOT, errors, crc, nbytes, transize : integer;
    i, temp : integer;
    data: byte;
    result : boolean;
    label done;

    begin
    Writeln;
    Write('Enter File to Send: ');
    ReadLn(fname);
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
    errors :=  0;
    Port[fCmd] := 1;
    delay(20);
    if not SendData($30) then
       begin
       errors := errors + 1 ;
       WriteLn('Command Send error');
       goto done;
       end;
    j := 1;
    repeat
        if SendData(byte(fname[j])) then
           j := j + 1
         else
           errors := errors + 1;
        if errors > 8 then
           begin
           WriteLn('Filename send error ', errors);
           goto done;
           end;
        until j > length(fname) ;
    if not SendData(0) then
        errors := errors + 1 ;
    BlockRead(fp,buffer, buffSize div 128,recs);

    (* send file *)
    snum :=1;
    transize := 128;
    errors := 0;
    bptr :=0;
    flagEOT := 0;
    nbytes := recs * 128;
    if nbytes < buffSize then
       flagEOT := 1;
    if nbytes = 0 then
       begin
       Writeln('Empty File');
       goto done;
       end;
    Writeln('Sending File');
    repeat
        if not ReadData(data) then
            errors := errors + 1
          else
            errors := 0;
        until (errors > 10) or (errors = 0);
{}    if errors > 10  then
       begin
       WriteLn('Filename Send ACK Error');
       goto done;
       end;
{ }
    if data = byte('C') then
       Writeln('Got CRC request: ', char(data))
     else
       begin
       Writeln('Wrong byte: ', data);
       goto done;
       end;
    Writeln;
    repeat
          Write(char(CR));
          Write('Sending Sector: ', snum);
          if not SendData(SOH) then
             errors := errors + 1;
          if not SendData(snum) then
             errors := errors + 1;
          if not SendData(not snum) then
             errors := errors + 1;
          crc := 0;
          bptrS := bptr;
          j := 0;
          repeat
              if SendData(buffer[bptr]) then
                 begin
                 crc := CrcCalc(crc, buffer[bptr]);
                 bptr := bptr + 1;
                 j := j + 1;
                 end
               else
                 errors := errors + 1;
              until (j = transize) or (errors > 10);
 {          Write(' End Sector, CRC: ', HexOut(crc),' 1: ',crc shr 8);
           WriteLn(' 2: ',crc and $ff);
 }
           if not SendData((crc shr 8) and $00ff) then
                 errors := errors + 1;
           if not SendData(crc and $ff) then
                 errors := errors + 1;
           j := 0;
           while not ReadData( data) and (j  < 5) do
               j := j + 1;
           if j = 5 then
               errors := errors + 1;
{Writeln('Data Check ', data);
 }
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
              bptr := bptrS;
              if data = NAK then
                 begin
                 WriteLn('Error: Bad sector # ',snum);
                 errors := errors + 1;
                 end
               else
                 begin
                 WriteLn('Error: Garbled ACK # ', snum);
                 errors := errors + 1;
                 end;
              end;
{          Writeln('Error count: ', errors);
}
          until (nbytes = 0) or (errors >10) ;
    result := SendData( EOT);
    if not ReadData(data) then;
{Writeln('data: ', data);
}
    Writeln;
    if data <> 6 then
       Writeln('Error: File transmission end error');
    if errors < 10 then
       WriteLn('Transfer Complete')
     else
       WriteLn('Abort transfer');
    done:
    Close(fp);
    end;
