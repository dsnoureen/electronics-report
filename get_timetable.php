<?php
header("Content-Type: application/json");

// Database connection
$host = "localhost";
$user = "root"; // change if needed
$pass = "";
$dbname = "med_schedule";

$conn = new mysqli($host, $user, $pass, $dbname);
if ($conn->connect_error) {
    http_response_code(500);
    echo json_encode(["error" => "Database connection failed"]);
    exit;
}

// Fetch current meal times (always one row)
$result = $conn->query("SELECT * FROM meal_times LIMIT 1");

if ($result && $result->num_rows > 0) {
    $data = $result->fetch_assoc();
    echo json_encode([
        "status" => "success",
        "timetable" => [
            "breakfast" => [
                "start" => $data['breakfast_start'],
                "end" => $data['breakfast_end']
            ],
            "lunch" => [
                "start" => $data['lunch_start'],
                "end" => $data['lunch_end']
            ],
            "dinner" => [
                "start" => $data['dinner_start'],
                "end" => $data['dinner_end']
            ],
            "updated_at" => $data['updated_at']
        ]
    ]);
} else {
    http_response_code(404);
    echo json_encode(["status" => "error", "message" => "No timetable found"]);
}

$conn->close();
?>
