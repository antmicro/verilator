class Base;
  int k = 7;

  function new(string foo);
    $write("%s\n", foo);
    fork
      $write("Do some forking\n");
    join_none
  endfunction

  function string get_val();
    Base b = this;
    return "Hello World";
  endfunction
endclass

class Derived extends Base;
  function new();
    super.new(get_val());
  endfunction
endclass

module t;
  Derived d;

  initial begin
    d = new;
    #1;
  end
endmodule
