#
# lese Daten vom Sensor in CSV Dateien
#

###############################################################################
#### configure                                                             ####
###############################################################################
$sensor = "esp-h2o-0000d42c.fritz.box"
$getTodayApi = "/api/v1/today"
$getPastApi = "/api/v1/data?from="
###############################################################################
###############################################################################


#
# erzeige die richtigen Zeichenketten
#
$today = Get-Date
$yesterday = $today.AddDays(-1)
$bYesterday = $today.AddDays(-2)
$todayString = $today.ToString("yyyy-MM-dd")
$yesterdayString = $yesterday.ToString("yyyy-MM-dd")
$bYesterdayString = $bYesterday.ToString("yyyy-MM-dd")

#
# lade die Daten herunter
#
$uri = "http://" + $sensor + $getTodayApi
$fileName = $todayString + "-pressure.csv"
Write-Host "Daten von heute: " $uri " ==> " $fileName
try {
    $Response = Invoke-WebRequest -Uri $uri
    $Stream = [System.IO.StreamWriter]::new('.\' + $fileName, $false)
    $Stream.Write($Response.Content)
}
catch {
    Write-Host "Datei von heute nicht vorhanden" 
}
finally {
    $Stream.Dispose()
}


#
# daten von gestern
#
$uri = "http://" + $sensor + $getPastApi + $yesterdayString
$fileName = $yesterdayString + "-pressure.csv"
Write-Host "Daten von gestern: " $uri " ==> " $fileName
try {
    $Response = Invoke-WebRequest -Uri $uri
    $Stream = [System.IO.StreamWriter]::new('.\' + $fileName, $false)
    $Stream.Write($Response.Content)
}
catch {
    Write-Host "Datei von gestern nicht vorhanden" 
}
finally {
    $Stream.Dispose()
}

#
# daten von Vorgestern
#
$uri = "http://" + $sensor + $getPastApi + $bYesterdayString
$fileName = $bYesterdayString + "-pressure.csv"
Write-Host "Daten von vorgestern: " $uri " ==> " $fileName
try {
    $Response = Invoke-WebRequest -Uri $uri
    $Stream = [System.IO.StreamWriter]::new('.\' + $fileName, $false)
    $Stream.Write($Response.Content)
}
catch {
    Write-Host "Datei von vorgestern nicht vorhanden" 
}
finally {
    $Stream.Dispose()
}

