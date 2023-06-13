program esp3;
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
    buf: array[1..10] of integer;
    error: integer;
var status : byte;

function HexOut(num:integer): str4; forward;
function SendData( data : byte ): boolean; forward;
function ReadData(Var data:byte): boolean; forward;

{$I espbase.pas}


(************** getStatus: byte ;            ***********)
function getStatus: byte ;
    begin
    getStatus := Port[fStat] and $3;
{    Writeln(HexOut(Port[fStat]));
{}
    end;

{************** Send Data ****************}
function SendData;
    var
        timeout, cnt: integer ;
        status : byte;
        result : boolean;
    begin
    cnt := 0;
    result := true;
    timeout := 1000;
    repeat
      status := getStatus;
      cnt := cnt +1;
      until (status = 2) or (status = 0) or (cnt > timeout);
{      Writeln('Data: ', HexOut(data),' Status: ', HexOut(status), ' Errors: ', cnt);
{}
    if cnt > timeout then
        begin
        result := false;
        {Writeln('Send Data Timeout error');
        }
        end
      else
        begin
        Port[fData] := data;
{        if (data < 10) or (data > 127) then
           begin
           WriteLn(data);
           end
         else
           Write(char(data));
 }
        end;
{}
Writeln('Data: ', HexOut(data),' Send Status: ',getStatus, ' Cnt: ', cnt);
{}
    SendData := result;
    end;


{************** Read Data ****************}
function ReadData;
{function ReadData(Var data:byte): boolean; forward;}

    var
        timeout, cnt : integer;
        stat1, stat2 : byte;
        err: boolean;
    begin
    cnt := 0;
    err  := True;
    timeout := 3000;
    stat2 := 0;
    repeat
      stat1 := getStatus;
      cnt := cnt +1;
{      if stat1 <> stat2 then
         begin
         Writeln('Old: ',stat2, ' New: ', stat1, 'cnt: ', cnt);
         stat2 := stat1;
         end;
{}
      until (stat1 = 1) or (cnt > timeout);
    if cnt > timeout then
        err := False
      else
        data := Port[fData];
    ReadData := err;
    end;



(************** get File ***************)
procedure getFile ;

const
     buffSize = 1024;    { must be a multiple of 128 }
var
    j, fsize: integer;
    fname: str20;
    fp: file;
    buffer: array[0..buffSize] of byte;
    bptr, recs: integer;
    start, snum, notsnum : byte;
    flagEOT, errors, err, nbytes, transize : integer;
    crc, crc1, crc2 : integer;
    i, temp : integer;
    data: byte;
    result : boolean;
    label done;

    begin
    Writeln;
    Write('Enter File to Get from ESP32: ');
    ReadLn(fname);
    Writeln('Asking for: ', fname);
    Port[fCmd] := 1;
    result := SendData($31);
    for j := 1 to length(fname) do
        result := SendData(byte(fname[j]));
    result := SendData(0);

{    if  not ReadData(data) then;
    if data = EOT then
        begin
        WriteLn('File not available');
        goto done;
        end;
 {}
    Assign(fp, fname);
    {$I-}
    Rewrite(fp);      (* test if file created *)
    {$I+}
    if IOresult <> 0 then
       begin
       Writeln('Can''t create file: ',fname);
       goto done;
       end;

    (* get file *)
    snum :=1;
    transize := 128;
    errors := 0;
    err := 0;
    bptr :=0;
    WriteLn('Sending ''C''');
    while not SendData(byte('C')) do
        begin
        err := err + 1;
        if err > 10 then
            begin
            WriteLn('Initial C error');
            goto done;
            end;
        end;
    err := 0;
    start := 255;
    if not ReadData(start) then
       err := err + 1;
    repeat
        snum := 255;
        notsnum := 255;
        if not ReadData(snum) then
           err := err + 1;
        if not ReadData(notsnum) then
           err := err + 1;
        if err > 0 then
           begin
           WriteLn('Header errors: ', err);
           WriteLn('SOH: ', start,' snum: ', snum,' ~snum ',notsnum);
           errors := errors + 1;
           goto done;
           end;
        crc := 0;
        if (start = SOH) and (snum + notsnum = 255 ) then
            begin
            Writeln('Good header: ',snum);
            temp := bptr;
            err := 0;
            repeat     { good packet header }
                if ReadData(buffer[bptr]) then
                   begin
                   crc := CrcCalc(crc, buffer[bptr]);
                   bptr := bptr + 1;
                   end
                 else
                   err := err + 1;
                until (bptr mod 128 = 0) and (err < 50) ;
            if err = 50 then
                begin
                WriteLn('Data Block error');
                goto done;
                end;
            err := 0;
            while (not ReadData(start)) and (err < 8) do
                err := err + 1;
            if err < 8 then
               crc1 := start
             else
               crc1 := 255;
            err := 0;
            while (not ReadData(start)) and (err < 8) do
               err := err + 1;
            if err < 8 then
                crc2 := start
              else
                crc2 := 255;
WriteLn('My CRC: ', HexOut(crc), ' crc1: ', HexOut(crc1), ' crc: ', HexOut(crc2));
            if( crc = (crc2 + crc1 shl 8) )then
                begin
                Writeln('CRC ok');
                err := 0;
                while ( not sendData(ACK) ) and (err < 10) do
                       err := err + 1;
                if err = 10 then
                   begin
                   WriteLn('ACK error');
                   errors := errors + 1;
                   end
                end
              else
                begin
                err := 0;
                WriteLn('CRC error');
                if sendData(NAK) then;
{                while (not sendData(NAK)) and (err < 10) do
                      err := err +1;
                if err = 10 then
                   WriteLn('NAK error');
 }
                bptr := temp;      { reset buffer ptr }
                errors := 10;
                end;
            if bptr = buffSize then
                begin
                BlockWrite(fp, buffer, buffSize div 128, recs );
                bptr := 0;
                end;
            end
          else
            errors := errors + 1;
        err := 0;
        start := 255;
        while (not  ReadData(start)) and (err < 8) do
            err := err + 1;
        if err = 8 then
            begin
            errors := 10;
            WriteLn('SOH error');
            end;
        Writeln('Loop End');
        until (start = EOT) or (errors > 8);
    if bptr > 0 then    { write rest of buffer }
       BlockWrite( fp, buffer, bptr div 128);
    close(fp);
    if errors < 8 then
       WriteLn('Transfer Complete')
     else
       WriteLn('Abort transfer');
    done:
    Close(fp);
    end;
{}

(******************* Main ******************)
var cmd: char;
    last: integer;
    j : integer;
begin
WriteLn('Welcome to ESP32 Test 3');
repeat
    Writeln;
    WriteLn('ESP3: Enter Command to Run (0 to end)');
    Writeln('1 : Debug');
    WriteLn('2 : Get File List');
    WriteLn('3 : Send File');
    WriteLn('4 : Get File');
    WriteLn('S : Current Status');
    Writeln('X: Reset ESP32');

    Read(cmd);
    cmd := UpCase(cmd);
    case cmd of
        '1': begin
             debug;
             end;
        '2': begin
             last := fileList;
             Writeln('Bytes: ', last);
             Writeln;
             end;
        '3': begin
             sendFile;
             end;
        '4': begin
             getFile;
             end;
        'X': begin
             Port[fCmd] := 1;
             Port[fCmd] := 1;
             end;
        'S': begin
             WriteLn('Status: ', getStatus);
             end;

        '0':
         else begin
              Writeln('Invalid Command');
              end;
         end;
    until cmd = '0' ;
end.
