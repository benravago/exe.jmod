package exe;

// import hello.Say;

public class Main {

  public static void main(String[] args) throws Exception {
    new Main().start(args);
  }

  void start(String[] args) throws Exception {

    System.out.println("--");
    for (var i = 0; i < args.length; i++) {
      System.out.println("arg #"+i+" '"+args[i]+"'");
    }

    System.out.println("--");
    System.getProperties().entrySet().stream()
      .sorted((a,b) -> a.getKey().toString().compareTo(b.getKey().toString()) )
      .forEach((e) -> System.out.println(""+e.getKey()+": "+e.getValue()) );

    System.out.println("--");
    System.out.println("LD_LIBRARY_PATH="+System.getenv("LD_LIBRARY_PATH"));

    // System.out.println("--");
    // new Say().hello();
  }
  
}
