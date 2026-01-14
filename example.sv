class Base;
  int k = 7;
  int queue[$];
  event a;

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
  int queue2[$] = {1,2,3};
  event b;

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
