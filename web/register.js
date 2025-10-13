window.onload = pageLoad;
function pageLoad(){
    var form = document.getElementById("myRegister")
    form.onsubmit = validateForm;
	
}

function validateForm() {
    // ดึงค่ามาตรวจสอบว่าพิมแล้วหรือนัง?
    var firstname  = document.forms["myRegister"]["firstname"].value;
    var lastname   = document.forms["myRegister"]["lastname"].value;
    var gender     = document.forms["myRegister"]["gender"].value;
    var bday       = document.forms["myRegister"]["bday"].value;

    if (!firstname || !lastname || !gender || !bday ) {
        document.getElementById("errormsg").innerText = "กรอกข้อมูลให้ครบจ้า";
        return false;
    } 
    

    return true;
}