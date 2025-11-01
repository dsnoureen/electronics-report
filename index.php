<?php
// Database Connection
$host = "localhost";
$user = "root";
$pass = "";
$dbname = "med_schedule";
$conn = new mysqli($host, $user, $pass, $dbname);
if ($conn->connect_error) die("Connection failed: " . $conn->connect_error);

// Handle API request for timetable JSON
if (isset($_GET['get_timetable'])) {
    header('Content-Type: application/json');
    $result = $conn->query("SELECT * FROM meal_times LIMIT 1");
    echo json_encode($result->fetch_assoc());
    exit;
}

// Handle Form Submission
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $b_start = $_POST['breakfast_start'];
    $b_end = $_POST['breakfast_end'];
    $l_start = $_POST['lunch_start'];
    $l_end = $_POST['lunch_end'];
    $d_start = $_POST['dinner_start'];
    $d_end = $_POST['dinner_end'];

    // Update existing record (only 1 row)
    $conn->query("UPDATE meal_times SET 
        breakfast_start='$b_start',
        breakfast_end='$b_end',
        lunch_start='$l_start',
        lunch_end='$l_end',
        dinner_start='$d_start',
        dinner_end='$d_end'
        WHERE id=1
    ");
    echo "<script>alert('Meal times updated successfully!'); window.location.href='medicine_scheduler.php';</script>";
}

// Fetch Current Meal Times
$result = $conn->query("SELECT * FROM meal_times LIMIT 1");
$meal = $result->fetch_assoc();
?>

<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>Meal Time Scheduler</title>
<style>
body {
    font-family: Arial, sans-serif;
    background: #f3f7fa;
    margin: 0;
    padding: 0;
}
.container {
    display: grid;
    grid-template-columns: 1fr 1fr;
    max-width: 900px;
    margin: 40px auto;
    background: #fff;
    border-radius: 12px;
    box-shadow: 0 2px 8px rgba(0,0,0,0.1);
    overflow: hidden;
}
.left {
    background: #2d89ef;
    color: white;
    padding: 30px;
}
.right {
    padding: 30px;
}
h1 {
    text-align: center;
}
form {
    display: flex;
    flex-direction: column;
}
label {
    margin-top: 15px;
    font-weight: bold;
}
input[type="time"] {
    padding: 8px;
    margin-top: 5px;
    border: 1px solid #ccc;
    border-radius: 6px;
}
button {
    margin-top: 20px;
    padding: 10px;
    border: none;
    border-radius: 6px;
    background: #2d89ef;
    color: white;
    font-size: 16px;
    font-weight: bold;
    cursor: pointer;
}
button:hover {
    background: #1b5fbf;
}
.schedule-display {
    background: white;
    color: #333;
    padding: 20px;
    border-radius: 8px;
}
.schedule-display h3 {
    border-bottom: 1px solid #ddd;
    padding-bottom: 6px;
    margin-bottom: 10px;
}
.schedule-display div {
    margin: 8px 0;
}
</style>
</head>
<body>

<div class="container">
    <div class="left">
        <h1>🍽️ Meal Time Routine</h1>
        <div class="schedule-display">
            <h3>Current Schedule</h3>
            <div><b>Breakfast:</b> <?= $meal['breakfast_start'] ?> - <?= $meal['breakfast_end'] ?></div>
            <div><b>Lunch:</b> <?= $meal['lunch_start'] ?> - <?= $meal['lunch_end'] ?></div>
            <div><b>Dinner:</b> <?= $meal['dinner_start'] ?> - <?= $meal['dinner_end'] ?></div>
        </div>
    </div>

    <div class="right">
        <h1>⏰ Update Timings</h1>
        <form method="POST">
            <label>Breakfast Start</label>
            <input type="time" name="breakfast_start" value="<?= $meal['breakfast_start'] ?>" required>

            <label>Breakfast End</label>
            <input type="time" name="breakfast_end" value="<?= $meal['breakfast_end'] ?>" required>

            <label>Lunch Start</label>
            <input type="time" name="lunch_start" value="<?= $meal['lunch_start'] ?>" required>

            <label>Lunch End</label>
            <input type="time" name="lunch_end" value="<?= $meal['lunch_end'] ?>" required>

            <label>Dinner Start</label>
            <input type="time" name="dinner_start" value="<?= $meal['dinner_start'] ?>" required>

            <label>Dinner End</label>
            <input type="time" name="dinner_end" value="<?= $meal['dinner_end'] ?>" required>

            <button type="submit">💾 Save Schedule</button>
        </form>
    </div>
</div>

</body>
</html>
