program esp;
{ comments}
type
    str20 = string[20];
    str4 = string[4] ;
const
     fData = $7c;
     fCmd = $7e;
     fStat = $7d;
var cnt : integer;
    dataCnt : integer;
    temp : integer;
    data: array[1..10] of integer;
    error: integer;
    ans: char;
var status : byte;

  (*********************** Hex Out *******************)

  function HexOut( num: integer ): str4;
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
    timeout := 5;
    repeat
      status := getStatus;
      cnt := cnt +1;
{      Writeln('Status: ', HexOut(status));
{}
      until (status = 2) or (cnt > timeout);
    if cnt > timeout then
        Writeln('Send Data Timeout error')
      else
        Port[fData] := data;
Writeln('Send Data Status: ',getStatus, ' Cnt: ', cnt);

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


(******************* Main ******************)
begin
dataCnt := 0;
error := 0;
WriteLn('Welcome to ESP32 Test');
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
    Writeln('Read Errors: ', error);
{
    Writeln('Status: ', getStatus);
    while (getStatus <> 2) and (cnt < 5) do
        begin
        Writeln( getStatus );
        cnt := cnt + 1;
        delay(5);
        end;

    Writeln('Cnt: ', cnt );
    if( cnt < 5 ) then
        begin
        temp := getStatus;
        Writeln('Loop Status: ',temp);
        Writeln('Sending 4 bytes starting with ', dataCnt);
        for cnt := 1 to 4 do
            begin
            SendData(dataCnt);
            dataCnt := dataCnt +1;
            end;
        end
      else
        Writeln('Timeout Error');
 {}
    Writeln('Send Command again?');
    Read(ans);
    ans := UpCase(ans);
    Writeln;
    until ans <> 'Y';
end.